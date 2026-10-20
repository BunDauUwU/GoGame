#include "MediumAI.h"
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace std;

namespace {
template<class T>
T calc(T &val, T Min, T Max)
{
    if (val > Max)
        return Max;
    if (val < Min)
        return Min;
    return val;
}

bool ownEye(const GoMatch &game, int pos, int color)
{
    const auto &board = game.cells();
    for (int next : GoMatch::neighbors()[pos])
        if (next >= 0 && board[next] != color)
            return false;
    int own = 0, off = 0;
    for (int dr : {-1, 1})
        for (int dc : {-1, 1}) {
            int r = pos / 19 + dr, c = pos % 19 + dc;
            if (r < 0 || r >= 19 || c < 0 || c >= 19)
                ++off;
            else if (board[r * 19 + c] == color)
                ++own;
        }
    return off ? own + off == 4 : own >= 3;
}
} // namespace

MediumAI::MediumAI(SearchOptions options)
    : m_options(options)
{
    m_options.maxDepth = calc(options.maxDepth, 1, 9);
    m_options.candidateLimit = calc(options.candidateLimit, 1, 24);
    m_options.timeBudgetMs = calc(options.timeBudgetMs, 1, 2000);
    m_options.stableIterations = calc(options.stableIterations, 0, 12);
    m_options.minimumStopDepth = calc(options.minimumStopDepth, 3, 9);
    m_options.improvementEpsilon = isfinite(options.improvementEpsilon)
                                       ? calc(options.improvementEpsilon, 0.0, 1.0)
                                       : 0.05;
    m_options.passMargin = isfinite(options.passMargin) ? calc(options.passMargin, 0.0, 1.0) : 0.0;
}

double MediumAI::evaluate(const GoMatch &game) const
{
    double val = game.score();
    if (game.consecutivePasses >= 2)
        return val;
    const auto &board = game.cells();
    const auto &groups = game.groups();
    array<int, GoMatch::Count> eyes{};
    array<double, GoMatch::Count> influence{};
    for (int pos = 0; pos < GoMatch::Count; ++pos) {
        if (board[pos] == -1) {
            for (int color : {0, 1})
                if (ownEye(game, pos, color)) {
                    for (int next : GoMatch::neighbors()[pos])
                        if (next >= 0) {
                            ++eyes[groups.root(next)];
                            break;
                        }
                    break;
                }
            continue;
        }
        const int sign = board[pos] == 0 ? 1 : -1;
        for (int dr = -2; dr <= 2; ++dr)
            for (int dc = -2; dc <= 2; ++dc) {
                int dist = abs(dr) + abs(dc);
                int r = pos / 19 + dr, c = pos % 19 + dc;
                if (dist > 0 && dist <= 2 && r >= 0 && r < 19 && c >= 0 && c < 19
                    && board[r * 19 + c] == -1)
                    influence[r * 19 + c] += double(sign) / dist;
            }
    }
    for (int pos = 0; pos < GoMatch::Count; ++pos) {
        if (board[pos] == -1) {
            val += calc(influence[pos], -1.0, 1.0) * 0.05;
            continue;
        }
        if (groups.root(pos) != pos || eyes[pos] >= 2)
            continue;
        const int sign = board[pos] == 0 ? 1 : -1;
        const auto count = groups.liberties[pos].count();
        if (count == 1)
            val -= sign * min(12.0, 0.5 * groups.size[pos]);
        else if (count == 2)
            val -= sign * min(2.0, 0.05 * groups.size[pos]);
    }
    return val;
}

class MediumAI::Search
{
public:
    struct Candidate
    {
        int pos;
        GoMatch next;
        double priorty;
        double value;
    };
    Search(const MediumAI &engine, const shared_ptr<atomic_bool> &token)
        : options(engine.options())
        , cancel(token)
        , m_engine(engine)
    {
        timer.start();
    }
    const SearchOptions &options;
    shared_ptr<atomic_bool> cancel;
    QElapsedTimer timer;
    int nodes = 0;
    bool incomplete = false;

private:
    const MediumAI &m_engine;

public:
    bool stopped()
    {
        if ((cancel && cancel->load(memory_order_relaxed))
            || timer.elapsed() >= options.timeBudgetMs)
            incomplete = true;
        return incomplete;
    }
    vector<Candidate> candidates(const GoMatch &game)
    {
        const auto &board = game.cells();
        const auto &groups = game.groups();
        const int color = game.currentPlayer;
        const int sign = color == 0 ? 1 : -1;
        array<bool, GoMatch::Count> nxt{};
        for (int r : {3, 9, 15})
            for (int c : {3, 9, 15})
                nxt[r * 19 + c] = true;
        // các nước gần quan -> quan trọng
        for (int pos = 0; pos < GoMatch::Count; ++pos)
            if (board[pos] >= 0)
                for (int dr = -2; dr <= 2; ++dr)
                    for (int dc = -2; dc <= 2; ++dc) {
                        int r = pos / 19 + dr, c = pos % 19 + dc;
                        if (r >= 0 && r < 19 && c >= 0 && c < 19)
                            nxt[r * 19 + c] = true;
                    }
        // các nước gần quân đã đặt -> quan trọng.

        struct Ranked
        {
            int pos;
            double priorty;
        };
        vector<Ranked> ranked;
        ranked.reserve(GoMatch::Count);
        for (int pos = 0; pos < GoMatch::Count; ++pos) {
            if (board[pos] != -1)
                continue;
            double priorty = nxt[pos] ? 0 : -4;
            bitset<GoMatch::Count> visited;
            for (int next : GoMatch::neighbors()[pos]) {
                if (next < 0)
                    continue;
                if (board[next] == -1) {
                    priorty += 0.1;
                    continue;
                }
                const int root = groups.root(next);
                if (visited.test(root))
                    continue;
                visited.set(root);
                const auto liberties = groups.liberties[root].count();
                if (liberties == 1)
                    priorty += (board[next] == color ? 3.0 : 4.0) * groups.size[root];
                else if (liberties == 2)
                    priorty += (board[next] == color ? 0.3 : 0.5) * groups.size[root];
            }
            if (ownEye(game, pos, color))
                priorty -= 2;
            int edge = min({pos / 19, pos % 19, 18 - pos / 19, 18 - pos % 19});
            if (game.moveCount < 30)
                priorty += edge == 3 ? 0.3 : edge == 2 ? 0.15 : edge == 0 ? -0.3 : 0;
            ranked.push_back({pos, priorty});
        }
        sort(ranked.begin(), ranked.end(), [](const Ranked &a, const Ranked &b) {
            return a.priorty == b.priorty ? a.pos < b.pos : a.priorty > b.priorty;
        });
        vector<Candidate> res;
        res.reserve(options.candidateLimit + 1);
        auto passed = game;
        passed.pass();
        double passValue = m_engine.evaluate(passed);
        res.push_back({-1, move(passed), sign * passValue, passValue});
        for (const auto &v : ranked) {
            if (stopped())
                break;
            auto next = game;
            QString error;
            if (!next.play(v.pos / 19, v.pos % 19, error))
                continue;
            double score = m_engine.evaluate(next);
            res.push_back({v.pos, move(next), sign * score, score});
            if (int(res.size()) > options.candidateLimit)
                break;
        }
        sort(res.begin(), res.end(), [](const Candidate &a, const Candidate &b) {
            return a.priorty == b.priorty ? a.pos < b.pos : a.priorty > b.priorty;
        });
        return res;
    }
    double minimax(const GoMatch &game, int depth, double alpha, double beta)
    {
        ++nodes;
        if (stopped())
            return 0;
        if (depth == 0 || game.consecutivePasses >= 2)
            return m_engine.evaluate(game);
        const bool maximize = game.currentPlayer == 0;
        double best = maximize ? -numeric_limits<double>::infinity()
                               : numeric_limits<double>::infinity();
        auto moves = candidates(game);
        for (const auto &move : moves) {
            if (stopped())
                return 0;
            double score;
            if (depth == 1 || move.next.consecutivePasses >= 2) {
                ++nodes;
                score = move.value;
            } else
                score = minimax(move.next, depth - 1, alpha, beta);
            if (incomplete)
                return 0;
            best = maximize ? max(best, score) : min(best, score);
            if (maximize)
                alpha = max(alpha, best);
            else
                beta = min(beta, best);
            if (beta <= alpha)
                break;
        }
        return best;
    }
};

SearchResult MediumAI::chooseMove(const GoMatch &cur, const shared_ptr<atomic_bool> &cancel) const
{
    Search search(*this, cancel);
    SearchResult res;
    if (cancel && cancel->load(memory_order_relaxed)) {
        res.cancelled = true;
        res.stopReason = QStringLiteral("cancelled");
        return res;
    }
    if (cur.consecutivePasses >= 2) {
        res.stopReason = QStringLiteral("terminal");
        return res;
    }
    const int sign = cur.currentPlayer == 0 ? 1 : -1;
    auto u = search.candidates(cur);
    const auto pass = find_if(u.begin(), u.end(), [](const auto &v) { return v.pos < 0; });
    res.passScore = sign * pass->value;
    res.estimatedScore = res.passScore;
    double fallback = -numeric_limits<double>::infinity();
    int fallbackMove = -1;
    for (const auto &v : u)
        if (v.pos >= 0 && sign * v.value > fallback) {
            fallback = sign * v.value;
            fallbackMove = v.pos;
        }
    const auto publish = [&](int chosen, double score, double passValue, double playValue) {
        res.pass = chosen < 0;
        res.row = chosen < 0 ? -1 : chosen / 19;
        res.col = chosen < 0 ? -1 : chosen % 19;
        res.estimatedScore = score;
        res.passScore = passValue;
        res.moveScore = isfinite(playValue) ? playValue : passValue;
        res.gainOverPass = res.moveScore - passValue;
    };
    if (fallbackMove >= 0 && fallback > res.passScore + m_options.passMargin + 1e-9)
        publish(fallbackMove, fallback, res.passScore, fallback);
    else
        publish(-1, res.passScore, res.passScore, fallback);
    struct Progress
    {
        int move = -2;
        double gain = 0;
        bool valid = false;
        int stalls = 0;
    };
    array<Progress, 2> progress;
    int lastMove = -2, stable = 0;
    const double infinity = numeric_limits<double>::infinity();
    for (int depth = 1; depth <= m_options.maxDepth; ++depth) {
        if (search.stopped())
            break;
        const double passBlack = depth == 1
                                     ? pass->value
                                     : search.minimax(pass->next, depth - 1, -infinity, infinity);
        if (search.incomplete)
            break;
        const double passValue = sign * passBlack;
        double alpha = -infinity, beta = infinity;
        if (sign > 0)
            alpha = passBlack + m_options.passMargin;
        else
            beta = passBlack - m_options.passMargin;
        double bestPlay = -infinity;
        int chosenMove = -1;
        for (const auto &v : u) {
            if (v.pos < 0)
                continue;
            if (search.stopped())
                break;
            double score = depth == 1 ? v.value : search.minimax(v.next, depth - 1, alpha, beta);
            if (search.incomplete)
                break;
            const double ownValue = sign * score;
            if (ownValue > bestPlay) {
                bestPlay = ownValue;
                chosenMove = v.pos;
            }
            if (sign > 0)
                alpha = max(alpha, score);
            else
                beta = min(beta, score);
        }
        if (search.incomplete)
            break;
        const bool choosePass = chosenMove < 0
                                || bestPlay <= passValue + m_options.passMargin + 1e-9;
        const int chosen = choosePass ? -1 : chosenMove;
        publish(chosen, choosePass ? passValue : bestPlay, passValue, bestPlay);
        res.completedDepth = depth;
        stable = chosen == lastMove ? stable + 1 : 1;
        lastMove = chosen;
        auto &previous = progress[depth % 2];
        const double gain = res.estimatedScore - passValue;
        const bool stalled = previous.valid && previous.move == chosen
                             && gain <= previous.gain + m_options.improvementEpsilon;
        previous.stalls = stalled ? previous.stalls + 1 : 0;
        previous.move = chosen;
        previous.gain = gain;
        previous.valid = true;
        if (m_options.stableIterations > 0 && depth >= m_options.minimumStopDepth
            && stable >= m_options.stableIterations && progress[0].stalls > 0
            && progress[1].stalls > 0) {
            res.stoppedEarly = true;
            res.stopReason = QStringLiteral("stable-or-declining"); // don't need to play
            break;
        }
    }
    res.cancelled = cancel && cancel->load(memory_order_relaxed);
    res.nodes = search.nodes;
    res.elapsedMs = search.timer.elapsed();
    if (res.cancelled)
        res.stopReason = QStringLiteral("cancelled");
    else if (res.stopReason.isEmpty())
        res.stopReason = search.incomplete ? QStringLiteral("time-budget")
                                           : QStringLiteral("max-depth");
    return res;
}
