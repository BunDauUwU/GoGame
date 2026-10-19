// Exercise the private board/arena with deterministic evaluators, not a model.
#define GOGAME_HARD_TESTS
#include "../src/AI/HardAI.cpp"
#include <QCoreApplication>
#include <random>
#include <cstdio>
#include <cstdlib>

static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
namespace { namespace HardMCTS {
struct Fixed : Evaluator {
    int calls = 0;
    Evaluation evaluate(const Board&, const Legal&, const Groups&) override {
        ++calls;
        Evaluation e; e.logits.fill(-20.f); e.logits[60] = 20.f; e.value = -0.7f;
        return e;
    }
};
struct EngineProbe {
    static void formulas() {
        Fixed evaluator; SearchOptions options; options.memoryMB = 4;
        Engine engine(evaluator, options); Board board;
        engine.resetTree(board); engine.expand(0, board);
        auto& node = engine.nodes_[0]; node.value = 0.3f; node.visits = 123;
        auto& first = engine.edges_[node.first];
        first.prior = 19000; first.child = uint32_t(engine.nodes_.size());
        Engine::Node child; child.visits = 5; child.sum = 1.5f;
        engine.nodes_.push_back(child);
        const double visitedPrior = double(first.prior) / 65535.;
        const double fpu = std::max(-1., double(node.value) - .2 * std::sqrt(visitedPrior));
        const double exploration = (1.5 + std::log1p(123. / 19652.)) * std::sqrt(123.);
        uint32_t expected = node.first; double best = -1e100;
        for (uint32_t i = node.first; i < node.first + node.count; ++i) {
            const auto& e = engine.edges_[i]; const auto n = engine.visits(e);
            const double q = n ? -1.5 / 5. : fpu;
            const double score = q + exploration * (double(e.prior) / 65535.) / (1. + n);
            if (score > best) { expected = i; best = score; }
        }
        require(engine.select(0) == expected, "PUCT/FPU formula or sign mismatch");
        require(sizeof(Engine::Edge) == 8, "Edge exceeded eight bytes");
        options.maxDepth = 1; options.maxSimulations = 1; options.timeMs = 1000;
        Engine shallow(evaluator, options);
        const auto result = shallow.search(board);
        require(result.rootValue > .69f && result.simulations == 1,
                "Depth boundary used zero instead of leaf value / incorrect backup sign");
    }
    static void cache() {
        Fixed evaluator; SearchOptions options; options.memoryMB = 4;
        Engine engine(evaluator, options); Board a, b;
        auto g = a.groups(); engine.evaluate(a, a.legalMoves(g), g);
        engine.evaluate(a, a.legalMoves(g), g);
        require(evaluator.calls == 1 && engine.hits_ == 1, "Exact cache hit not reused");
        b.setStone(0, 0, 1);
        b.stonesHash = a.stonesHash; // Deliberate 64-bit hash collision.
        require(a.key() == b.key() && !a.same(b), "Collision fixture invalid");
        g = b.groups(); engine.evaluate(b, b.legalMoves(g), g);
        require(evaluator.calls == 2, "Hash collision reused wrong policy/value");
        for (int field = 0; field < 6; ++field) {
            b = a;
            if (field == 0) b.turn = 2;
            if (field == 1) b.ko = padded(60);
            if (field == 2) b.passes = 1;
            if (field == 3) b.komi = 7.5;
            if (field == 4) b.capturedStones[0] = 1;
            if (field == 5) b.moveCount = 1;
            const auto calls = evaluator.calls;
            g = b.groups(); engine.evaluate(b, b.legalMoves(g), g);
            require(evaluator.calls == calls + 1, "State metadata omitted from cache identity");
        }
    }
    static void reuseAndArena() {
        Fixed evaluator; SearchOptions options; options.memoryMB = 4;
        options.timeMs = 1000; options.maxSimulations = 1000; options.maxDepth = 12;
        Engine engine(evaluator, options); Board b;
        auto result = engine.search(b);
        require(result.simulations > 10, "Reuse test did not build a tree");
        const auto bytes = result.allocatedBytes;
        require(bytes <= 4 * 1024 * 1024 && bytes > 0, "Arena exceeds budget");
        require(engine.advance(result.move), "Own move advance failed");
        const auto& node = engine.nodes_[engine.root_];
        require(node.first != None, "Chosen subtree was not retained");
        uint32_t best = node.first;
        for (uint32_t i = node.first; i < node.first + node.count; ++i)
            if (engine.visits(engine.edges_[i]) > engine.visits(engine.edges_[best])) best = i;
        require(engine.visits(engine.edges_[best]) > 0, "Opponent subtree missing");
        require(engine.advance(engine.edges_[best].move), "Opponent move advance failed");
        b = engine.rootBoard();
        const auto visits = engine.nodes_[engine.root_].visits;
        const auto oldNodes = engine.nodes_.size();
        engine.compact();
        require(engine.root_ == 0 && engine.nodes_[0].visits == visits
                && engine.nodes_.size() < oldNodes, "Compaction discarded retained search");
        require(engine.board_.same(b) && engine.allocatedBytes() == bytes, "Compaction changed board or allocated RAM");
        for (uint32_t i = 0; i < engine.nodes_.size(); ++i) {
            const auto& n = engine.nodes_[i];
            if (n.first == None) continue;
            require(n.first + n.count <= engine.edges_.size(), "Compacted edge range corrupt");
            for (uint32_t j = n.first; j < n.first + n.count; ++j)
                require(engine.edges_[j].child == None ||
                        (engine.edges_[j].child > i && engine.edges_[j].child < engine.nodes_.size()),
                        "Compacted child index corrupt");
        }
        result = engine.search(b);
        require(result.reusedVisits == visits, "Own + opponent branch visits not reused");
        b.capturedStones[1]++; // Same stones/turn are insufficient to identify state.
        result = engine.search(b);
        require(result.reusedVisits == 0, "Incompatible root reused");

        // Remove the model's directional prior to fill the small arena broadly.
        struct Uniform : Evaluator {
            Evaluation evaluate(const Board&, const Legal&, const Groups&) override { return {}; }
        } uniform;
        options.timeMs = 3000; options.maxSimulations = 100000; options.maxDepth = 32;
        Engine full(uniform, options);
        result = full.search(Board{});
        require(result.memoryLimited && result.allocatedBytes <= 4 * 1024 * 1024,
                "Memory cap not enforced when arena fills");
        const auto move = result.move;
        require(full.advance(move), "Cannot retain reply at memory cap");
        result = full.search(full.rootBoard());
        require(result.reusedVisits > 0, "Memory pressure discarded compatible subtree");
    }
};
} }

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace HardMCTS;
    require(mix64(0) == 0xe220a8397b1dcdafULL, "SplitMix64 mapping changed");
    Board board; GoMatch reference; QString error;
    std::mt19937 rng(4281); int checked = 0;
    for (int step = 0; step < 210; ++step) {
        const auto g = board.groups(); const auto legal = board.legalMoves(g);
        for (int m = 0; m < Points; ++m) {
            if (step % 7 == 0) {
                require(bool(legal[m]) == reference.validMove(m / Size, m % Size), "Hard rule/legal mismatch");
                ++checked;
            }
            require(int(board.cells[padded(m)]) - 1 == reference.cells()[m], "Hard capture/board mismatch");
        }
        require(board.capturedStones == reference.capturedStones && board.moveCount == reference.moveCount
                && board.turn - 1 == reference.currentPlayer, "Hard metadata mismatch");
        require(std::abs(board.finalScore() - reference.score()) < 1e-8, "Hard final scoring mismatch");
        uint64_t hash = 0;
        for (int m = 0; m < Points; ++m)
            if (board.cells[padded(m)]) hash ^= stoneKey(padded(m), board.cells[padded(m)]);
        require(board.stonesHash == hash, "Incremental stone hash mismatch");
        int move = int(rng() % Points);
        while (!legal[move]) move = (move + 1) % Points;
        require(reference.play(move / Size, move % Size, error), "Reference rejected legal move");
        board.playKnownLegal(move);
    }
    Board ko; GoMatch koRef;
    for (auto pos : {std::pair<int,int>{0,1}, {0,2}, {1,0}, {1,1}, {2,1}, {2,2}, {10,10}, {1,3}, {1,2}}) {
        require(ko.play(pos.first*19+pos.second) && koRef.play(pos.first,pos.second,error), "Ko fixture failed");
    }
    require(!ko.legal(20) && ko.ko == padded(20), "Immediate ko recapture allowed");
    ko.playKnownLegal(Pass); koRef.pass();
    ko.playKnownLegal(Pass); koRef.pass();
    require(ko.terminal() && !ko.legal(Pass) && std::abs(ko.finalScore()-koRef.score())<1e-8,
            "Pass/terminal mismatch");
    EngineProbe::formulas(); EngineProbe::cache(); EngineProbe::reuseAndArena();
    std::printf("Hard core: %d legal comparisons, hashes, ko, scoring, PUCT/FPU, cache collision, reuse and RAM cap passed\n", checked);
}
