#include "../src/AI/HardAI.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <random>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
static void check(bool yes, const char* text) { if (!yes) { std::fprintf(stderr,"%s\n",text); std::exit(1); } }
struct Probe : HardAI::Evaluator {
    int calls = 0;
    HardAI::Position first;
    HardAI::Evaluation evaluate(const HardAI::Position& p, const HardAI::LegalMask& legal, const std::atomic_bool*) override {
        if (!calls) first = p;
        ++calls;
        check(legal[361] == 1, "Pass missing in legal mask");
        for (int i = 0; i < 361; ++i)
            check(p.cells[i] == -1 || !legal[i], "Occupied point legal in backend mask");
        HardAI::Evaluation e;
        e.policyLogits.fill(-10); e.policyLogits[60] = 5; e.value = 0;
        return e;
    }
};
struct Failure : HardAI::Evaluator {
    HardAI::Evaluation evaluate(const HardAI::Position&, const HardAI::LegalMask&, const std::atomic_bool*) override {
        throw std::runtime_error("model unavailable");
    }
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    HardSearchOptions options; options.timeBudgetMs=25; options.maxSimulations=32; options.memoryMB=4;
    HardAI hard(options);
    GoMatch board;
    const auto original = board.toJson();
    auto result=hard.chooseMove(board);
    check(!result.cancelled && result.error.isEmpty() && result.nodes>0,"Hard search did not run");
    check(result.pass || board.validMove(result.row,result.col),"Illegal initial Hard move");
    check(board.toJson()==original,"Hard mutated caller position");
    check(result.allocatedBytes > 0 && result.allocatedBytes <= 4 * 1024 * 1024,
          "Public search memory diagnostic outside configured budget");
    HardSearchOptions zero = options; zero.timeBudgetMs = 0;
    HardAI instant(zero);
    auto fallback = instant.chooseMove(board);
    check(fallback.nodes == 0 && fallback.evaluations == 0 && fallback.error.isEmpty()
          && (fallback.pass || board.validMove(fallback.row, fallback.col)), "Zero budget fallback invalid");
    auto token=std::make_shared<std::atomic_bool>(true);
    check(hard.chooseMove(board,token).cancelled,"Hard ignored pre-cancel");
    auto probe=std::make_shared<Probe>();hard.setEvaluator(probe);
    result=hard.chooseMove(board);
    check(result.error.isEmpty() && probe->calls>0,"Model adapter not called");
    check(result.row==3 && result.col==3 && !result.pass,"Policy logits not used");
    check(probe->first.currentPlayer==0 && probe->first.cells[0]==-1,"Backend color encoding wrong");
    auto planes=HardAI::inputPlanes(probe->first,HardAI::LegalMask{});
    check(planes[3*361]==1.f && planes[4*361]<0,"Black feature planes wrong");
    probe->first.currentPlayer=1;
    planes=HardAI::inputPlanes(probe->first,HardAI::LegalMask{});
    check(planes[3*361]==0.f && planes[4*361]>0,"White feature planes wrong");
    hard.setEvaluator(std::make_shared<Failure>());
    result=hard.chooseMove(board);
    check(result.error=="model unavailable","Model error silently played a move");
    hard.setEvaluator({});
    struct CancelDuringEvaluation : HardAI::Evaluator {
        std::shared_ptr<std::atomic_bool> token;
        int rootCalls = 0;
        HardAI::Evaluation evaluate(const HardAI::Position& p, const HardAI::LegalMask&, const std::atomic_bool*) override {
            if (p.moveCount == 0 && ++rootCalls == 1) token->store(true);
            return {};
        }
    };
    auto cancelModel = std::make_shared<CancelDuringEvaluation>();
    cancelModel->token = std::make_shared<std::atomic_bool>(false);
    hard.setEvaluator(cancelModel);
    result = hard.chooseMove(board, cancelModel->token);
    check(result.cancelled && result.error.isEmpty(), "Cancelled evaluation reported as a backend error");
    cancelModel->token->store(false);
    result = hard.chooseMove(board, cancelModel->token);
    check(!result.cancelled && result.error.isEmpty() && cancelModel->rootCalls == 2,
          "Cancelled evaluation polluted the cache");
    struct NonFinite : HardAI::Evaluator {
        HardAI::Evaluation evaluate(const HardAI::Position&, const HardAI::LegalMask&, const std::atomic_bool*) override {
            HardAI::Evaluation e; e.value = std::numeric_limits<float>::quiet_NaN();
            e.policyLogits.fill(e.value); return e;
        }
    };
    hard.setEvaluator(std::make_shared<NonFinite>());
    result = hard.chooseMove(board);
    check(result.error.isEmpty() && std::isfinite(result.winValue)
          && (result.pass || board.validMove(result.row, result.col)), "Non-finite evaluator output not sanitized");
    hard.setEvaluator({});
    // Play legal random positions and compare the returned action with actual GoMatch.
    std::mt19937 rng(42); QString error;
    for(int i=0;i<80;++i) {
        bool moved=false;
        for(int j=0;j<361;++j) {
            int point=int(rng()%361);
            if(board.play(point/19,point%19,error)){moved=true;break;}
        }
        if(!moved) break;
        if(i%8==0){
            result=hard.chooseMove(board);
            check(result.error.isEmpty() && (result.pass || board.validMove(result.row,result.col)),"Illegal midgame Hard move");
        }
    }
    board.pass();board.pass();
    result=hard.chooseMove(board);
    check(result.nodes==0 && result.pass,"Searched a finished game");
    std::puts("HardAI adapter, legality, cancellation, errors and terminal tests passed");
}
