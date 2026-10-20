#include "../src/AI/HardAI.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
static void check(bool ok, const char* message) { if (!ok) { std::fprintf(stderr,"%s\n",message); std::exit(1); } }
struct Concurrent : HardAI::Evaluator {
    std::atomic_int active{0}, peak{0}, calls{0};
    std::atomic_bool fail{false};
    std::shared_ptr<std::atomic_bool> token;
    HardAI::Evaluation evaluate(const HardAI::Position& p, const HardAI::LegalMask&, const std::atomic_bool*) override {
        check(p.history && p.history->setup, "Missing model history");
        const auto n = ++active; auto old = peak.load(); while (n > old && !peak.compare_exchange_weak(old,n)) {}
        ++calls;
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        --active;
        if (token && calls >= 5) token->store(true);
        if (fail && p.moveCount > 1) throw std::runtime_error("parallel model failure");
        HardAI::Evaluation e; e.policyLogits.fill(0); e.value = -.2f; return e;
    }
};
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    HardSearchOptions options; options.searchThreads = 4; options.timeBudgetMs = 1000; options.maxSimulations = 32; options.memoryMB = 4;
    HardAI hard(options); auto model = std::make_shared<Concurrent>(); hard.setEvaluator(model);
    GoMatch board; QString error;
    auto result = hard.chooseMove(board);
    check(result.error.isEmpty() && result.nodes == 32, "Parallel simulation budget incorrect");
    check(model->peak > 1 && model->active == 0, "Leaves not evaluated concurrently / workers leaked");
    check(result.pass || board.validMove(result.row,result.col), "Illegal parallel move");
    check(board.play(3,3,error) && board.play(15,15,error), "History fixture failed");
    const auto saved = board.toJson();
    GoMatch restored;
    check(GoMatch::fromJson(saved, restored, error) && restored.toJson() == saved, "History save/load mismatch");
    check(restored.replay().moves.size() == 2, "Real history missing");
    auto copy = board; check(copy.play(3,15,error), "Copy fixture failed");
    check(board.replay().moves.size() == 2 && copy.replay().moves.size() == 3, "Copied positions share mutable history");
    auto malformed = saved; auto h = malformed["history"].toObject(); h["moves"] = QJsonArray{}; malformed["history"] = h;
    check(!GoMatch::fromJson(malformed,restored,error), "Forged history accepted");
    auto legacy = saved; legacy.remove("history");
    check(GoMatch::fromJson(legacy,restored,error) && restored.replay().moves.empty()
        && restored.replay().initialCells == board.cells(), "Legacy save invented moves");
    model->fail = true; hard.reset(); result = hard.chooseMove(board);
    check(result.error == "parallel model failure" && model->active == 0, "Worker exception not propagated / joined");
    model->fail = false; hard.reset(); result = hard.chooseMove(board);
    check(result.error.isEmpty() && result.nodes == 32, "Worker error poisoned later searches");
    model->token = std::make_shared<std::atomic_bool>(false); hard.reset();
    result = hard.chooseMove(board, model->token);
    check(result.cancelled && result.error.isEmpty() && model->active == 0, "Parallel cancellation unsafe");
    model->token.reset(); hard.reset(); result = hard.chooseMove(board);
    check(result.error.isEmpty(), "Cancellation poisoned subsequent search");
    std::puts("Concurrent leaves, budgets, exceptions, cancellation and history tests passed");
}
