#include "../src/AI/KataGoEvaluator.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <random>
static void check(bool ok, const char* message) { if (!ok) { std::fprintf(stderr,"%s\n",message); std::exit(1); } }
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) { std::puts("Supply a KataGo model path to run native inference tests"); return 77; }
    KataGoEvaluator::Options backend; backend.modelPath = QString::fromLocal8Bit(argv[1]);
    backend.duty = argc > 2 ? QString::fromLocal8Bit(argv[2]).toDouble() : .30;
    backend.fp16 = !(argc > 4 && QString::fromLocal8Bit(argv[4]) == "fp32");
    auto model = std::make_shared<KataGoEvaluator>(backend);
    model->prepare();
    HardSearchOptions options; options.searchThreads = 4; options.timeBudgetMs = 1800; options.maxSimulations = 128;
    if (argc > 3) options.maxSimulations = QString::fromLocal8Bit(argv[3]).toUInt();
    HardAI hard(options); hard.setEvaluator(model);
    GoMatch board; QString error;
    std::mt19937 rng(41);
    for (int turn = 0; turn < 4; ++turn) {
        auto result = hard.chooseMove(board);
        if (!result.error.isEmpty()) std::fprintf(stderr, "%s\n", qPrintable(result.error));
        check(result.error.isEmpty() && !result.cancelled, "Native model search failed");
        check(std::isfinite(result.winValue) && std::abs(result.winValue) <= 1., "Invalid native value");
        check(result.evaluations > 0 && result.nodes > 0, "Native model not evaluated");
        if (result.pass) board.pass();
        else check(board.play(result.row, result.col, error), "Native MCTS chose an illegal move");
        const auto stats = model->statistics();
        std::printf("turn=%d move=%d,%d sims=%d evals=%llu ms=%lld value=%.5f rows=%llu batches=%llu hits=%llu reused=%u\n",
            turn, result.row, result.col, result.nodes, (unsigned long long)result.evaluations,
            (long long)result.elapsedMs, result.winValue, (unsigned long long)stats.rows,
            (unsigned long long)stats.batches, (unsigned long long)stats.hits, result.reusedVisits);
        GoMatch restored;
        check(GoMatch::fromJson(board.toJson(), restored, error), "Native game's saved history failed to load");
        check(restored.replay().moves.size() == board.replay().moves.size(), "Native history lost on save/load");
        bool moved = false;
        for (int j = 0; j < 361; ++j) { int point = int(rng()%361); if (board.play(point/19,point%19,error)) { moved = true; break; } }
        if (!moved) board.pass();
    }
    // A real capture followed by a ko ban, with a non-empty setup root.
    GoMatch ko;
    auto fixture = ko.toJson(); fixture.remove("history");
    QJsonArray cells; for (int i = 0; i < 361; ++i) cells.append(-1);
    for (int i : {1,19,39}) cells[i] = 0;
    for (int i : {20,2,40,22}) cells[i] = 1;
    fixture["board"] = cells; fixture["previousBoard"] = cells;
    check(GoMatch::fromJson(fixture,ko,error) && ko.play(1,2,error), "Ko fixture invalid");
    check(ko.koPoint == 20 && ko.capturedStones[0] == 1 && !ko.validMove(1,1), "Capture/ko not created");
    hard.reset(); auto koMove = hard.chooseMove(ko);
    if (!koMove.error.isEmpty()) std::fprintf(stderr,"%s\n",qPrintable(koMove.error));
    check(koMove.error.isEmpty() && (koMove.pass || ko.validMove(koMove.row,koMove.col)), "Native capture/ko search failed");
    // Keep a genuine pass in the history without ending the game.
    ko.pass(); hard.reset(); auto passed = hard.chooseMove(ko);
    check(passed.error.isEmpty() && (passed.pass || ko.validMove(passed.row,passed.col)), "Native pass history failed");
    auto token = std::make_shared<std::atomic_bool>(true);
    check(hard.chooseMove(board, token).cancelled, "Native pre-cancel ignored");
    const auto stats = model->statistics();
    check(stats.rows > 0 && stats.batches > 0 && stats.rows >= stats.batches, "Invalid native batching statistics");
    std::puts("Native KataGo model, MCTS, save/load, capture, ko and pass tests passed");
}
