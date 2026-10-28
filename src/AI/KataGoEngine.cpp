#include "KataGoEngine.h"
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QTemporaryFile>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {
QString vertex(int point)
{
    if (point == GoMatch::Count)
        return QStringLiteral("pass");
    return QString(QChar('A' + point % 19 + (point % 19 >= 8)))
           + QString::number(19 - point / 19);
}
QJsonObject request(const GoMatch &position, int timeBudgetMs)
{
    const auto history = position.replay();
    if (history.initialKo >= 0 || history.initialPasses != 0)
        throw std::runtime_error("This legacy setup lacks the history needed to restore ko/passes in KataGo");
    QJsonArray initial, moves, legal;
    for (int point = 0; point < GoMatch::Count; ++point) {
        if (history.initialCells[point] >= 0)
            initial.append(QJsonArray{history.initialCells[point] == 0 ? "B" : "W", vertex(point)});
        if (position.validMove(point / 19, point % 19))
            legal.append(vertex(point));
    }
    legal.append("pass");
    for (const auto &move : history.moves)
        moves.append(QJsonArray{move.player == 0 ? "B" : "W", vertex(move.point)});
    const QJsonObject rules{{"ko", "SIMPLE"}, {"scoring", "TERRITORY"},
                            {"tax", "NONE"}, {"suicide", false}, {"hasButton", false},
                            {"whiteHandicapBonus", "0"}, {"friendlyPassOk", false}};
    return {{"initialStones", initial}, {"moves", moves}, {"rules", rules},
            {"initialPlayer", history.initialPlayer == 0 ? "B" : "W"},
            {"komi", 6.5 + history.initialCaptured[1] - history.initialCaptured[0]},
            {"boardXSize", 19}, {"boardYSize", 19},
            {"allowMoves", QJsonArray{QJsonObject{{"player", position.currentPlayer == 0 ? "B" : "W"},
                                                 {"moves", legal}, {"untilDepth", 1}}}},
            {"overrideSettings", QJsonObject{{"maxTime", timeBudgetMs / 1000.0}}}};
}
}

struct KataGoEngine::Impl
{
    struct Job
    {
        QJsonObject query;
        int timeoutMs;
        std::shared_ptr<std::atomic_bool> cancel;
        std::promise<QJsonObject> result;
    };
    Options options;
    std::mutex mutex;
    std::condition_variable ready;
    std::deque<std::shared_ptr<Job>> jobs;
    std::atomic_bool stopping{false};
    std::thread worker;

    explicit Impl(Options value) : options(std::move(value)), worker([this] { run(); }) {}
    ~Impl()
    {
        stopping = true;
        ready.notify_all();
        worker.join();
    }
    QJsonObject submit(QJsonObject query, int timeoutMs,
                       const std::shared_ptr<std::atomic_bool> &cancel = {})
    {
        auto job = std::make_shared<Job>();
        job->query = std::move(query);
        job->timeoutMs = timeoutMs;
        job->cancel = cancel;
        auto result = job->result.get_future();
        {
            std::lock_guard<std::mutex> lock(mutex);
            jobs.push_back(job);
        }
        ready.notify_one();
        while (result.wait_for(std::chrono::milliseconds(20)) != std::future_status::ready)
            if (cancel && cancel->load())
                return {};
        return result.get();
    }
    void run()
    {
        QProcess process;
        QTemporaryFile config;
        QByteArray output, errors;
        quint64 sequence = 0;
        const auto stop = [&] {
            process.kill();
            process.waitForFinished(3000);
            output.clear();
        };
        const auto send = [&](const QJsonObject &query) {
            const auto line = QJsonDocument(query).toJson(QJsonDocument::Compact) + '\n';
            if (process.write(line) != line.size())
                throw std::runtime_error("Cannot write to KataGo");
        };
        for (;;) {
            std::shared_ptr<Job> job;
            {
                std::unique_lock<std::mutex> lock(mutex);
                ready.wait(lock, [&] { return stopping || !jobs.empty(); });
                if (stopping)
                    break;
                job = jobs.front();
                jobs.pop_front();
            }
            try {
                if (job->cancel && job->cancel->load()) {
                    job->result.set_value({});
                    continue;
                }
                if (process.state() == QProcess::NotRunning) {
                    errors.clear();
                    output.clear();
                    QString configPath = options.configPath;
                    if (configPath.isEmpty()) {
                        if (!config.isOpen()) {
                            if (!config.open())
                                throw std::runtime_error("Cannot create KataGo config");
                            const auto text = QStringLiteral(
                                "logToStderr = true\nreportAnalysisWinratesAs = SIDETOMOVE\n"
                                "numAnalysisThreads = 1\nnumSearchThreadsPerAnalysisThread = %1\n"
                                "nnMaxBatchSize = %1\nnnCacheSizePowerOfTwo = 16\nnnMutexPoolSizePowerOfTwo = 8\n"
                                "maxVisits = 100000\ncudaUseFP16 = %2\n")
                                                  .arg(options.searchThreads)
                                                  .arg(options.fp16 ? "auto" : "false").toUtf8();
                            if (config.write(text) != text.size() || !config.flush())
                                throw std::runtime_error("Cannot write KataGo config");
                        }
                        configPath = config.fileName();
                    }
                    process.start(options.executablePath,
                                  {"analysis", "-model", options.modelPath, "-config", configPath,
                                   "-override-config", "reportAnalysisWinratesAs=SIDETOMOVE",
                                   "-quit-without-waiting"});
                    if (!process.waitForStarted(5000))
                        throw std::runtime_error(process.errorString().toStdString());
                }
                const auto id = QString::number(++sequence);
                job->query["id"] = id;
                send(job->query);
                QElapsedTimer elapsed, termination;
                elapsed.start();
                bool done = false;
                while (!done) {
                    const bool cancelled = stopping || (job->cancel && job->cancel->load());
                    if (cancelled && !termination.isValid()) {
                        send({{"id", id + "-stop"}, {"action", "terminate"}, {"terminateId", id}});
                        termination.start();
                    }
                    if (elapsed.elapsed() > job->timeoutMs || (termination.isValid() && termination.elapsed() > 1500)) {
                        stop();
                        if (!cancelled)
                            throw std::runtime_error("KataGo analysis timed out");
                        job->result.set_value({});
                        break;
                    }
                    process.waitForReadyRead(20);
                    output += process.readAllStandardOutput();
                    errors = (errors + process.readAllStandardError()).right(8192);
                    if (output.size() > 4 * 1024 * 1024)
                        throw std::runtime_error("KataGo response is too large");
                    int end;
                    while ((end = output.indexOf('\n')) >= 0) {
                        const auto line = output.left(end);
                        output.remove(0, end + 1);
                        QJsonParseError error;
                        const auto document = QJsonDocument::fromJson(line, &error);
                        if (error.error != QJsonParseError::NoError || !document.isObject())
                            throw std::runtime_error("Invalid JSON from KataGo");
                        const auto reply = document.object();
                        if (reply.contains("error") && (!reply.contains("id") || reply["id"] == id))
                            throw std::runtime_error(reply["error"].toString().toStdString());
                        if (reply["id"] != id || reply.contains("warning") || reply["isDuringSearch"].toBool())
                            continue;
                        job->result.set_value(cancelled ? QJsonObject{} : reply);
                        done = true;
                        break;
                    }
                    if (!done && process.state() == QProcess::NotRunning)
                        throw std::runtime_error(("KataGo exited: " + QString::fromUtf8(errors)).toStdString());
                }
            } catch (...) {
                stop();
                job->result.set_exception(std::current_exception());
            }
        }
        stop();
        for (const auto &job : jobs)
            job->result.set_exception(std::make_exception_ptr(std::runtime_error("KataGo stopped")));
    }
};

KataGoEngine::KataGoEngine(Options options) : m_options(std::move(options))
{
    m_options.executablePath = QFileInfo(m_options.executablePath).absoluteFilePath();
    m_options.modelPath = QFileInfo(m_options.modelPath).absoluteFilePath();
    if (!m_options.configPath.isEmpty())
        m_options.configPath = QFileInfo(m_options.configPath).absoluteFilePath();
    if (!QFileInfo(m_options.executablePath).isExecutable() || !QFileInfo(m_options.modelPath).isFile()
        || (!m_options.configPath.isEmpty() && !QFileInfo(m_options.configPath).isFile())
        || m_options.timeBudgetMs < 1 || m_options.timeBudgetMs > 60000
        || m_options.searchThreads < 1 || m_options.searchThreads > 16)
        throw std::invalid_argument("Invalid KataGo executable, model, config, or search options");
    m_impl = std::make_unique<Impl>(m_options);
}
KataGoEngine::~KataGoEngine() = default;
QString KataGoEngine::name() const { return QStringLiteral("KataGo external engine"); }
void KataGoEngine::prepare()
{
    auto query = request(GoMatch(), 1);
    query["maxVisits"] = 1;
    const auto reply = m_impl->submit(query, 120000);
    if (reply["moveInfos"].toArray().isEmpty())
        throw std::runtime_error("KataGo warm-up returned no moves");
}
SearchResult KataGoEngine::chooseMove(const GoMatch &position,
                                     const std::shared_ptr<std::atomic_bool> &cancel)
{
    SearchResult result;
    QElapsedTimer elapsed;
    elapsed.start();
    try {
        if (cancel && cancel->load()) {
            result.cancelled = true;
            return result;
        }
        if (position.m_passes >= 2) {
            result.stopReason = "Game ended";
            return result;
        }
        const auto reply = m_impl->submit(request(position, m_options.timeBudgetMs),
                                          m_options.timeBudgetMs + 5000, cancel);
        if (cancel && cancel->load()) {
            result.cancelled = true;
        } else {
            const auto infos = reply["moveInfos"].toArray();
            if (infos.isEmpty())
                throw std::runtime_error("KataGo returned no moves");
            QJsonObject best;
            for (const auto &value : infos) {
                const auto info = value.toObject();
                if (best.isEmpty() || info["order"].toInt(1000000) < best["order"].toInt(1000000))
                    best = info;
            }
            const auto move = best["move"].toString().toUpper();
            result.pass = move == "PASS";
            if (!result.pass) {
                const auto columns = QStringLiteral("ABCDEFGHJKLMNOPQRST");
                bool ok = false;
                const int row = 19 - move.mid(1).toInt(&ok);
                const int col = move.isEmpty() ? -1 : columns.indexOf(move[0]);
                if (!ok || row < 0 || row >= 19 || col < 0 || !position.validMove(row, col))
                    throw std::runtime_error("KataGo returned an illegal move");
                result.row = row;
                result.col = col;
            }
            const auto root = reply["rootInfo"].toObject();
            result.rootVisits = root["visits"].toInt();
            result.nodes = int(result.rootVisits);
            result.winValue = 2 * best["winrate"].toDouble(0.5) - 1;
            result.estimatedScore = best["scoreLead"].toDouble() * (position.currentPlayer == 0 ? 1 : -1);
            result.moveScore = result.estimatedScore;
            result.stopReason = "KataGo analysis";
        }
    } catch (const std::exception &error) {
        result.error = QString::fromUtf8(error.what());
        result.cancelled = cancel && cancel->load();
    }
    result.elapsedMs = elapsed.elapsed();
    return result;
}
