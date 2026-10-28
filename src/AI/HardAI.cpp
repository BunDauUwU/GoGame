// AI-assisted: see AI13 in docs/AI_Declaration.md (MCTS/PUCT/FPU, cache, arena and subtree reuse).
#include "HardAI.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

// Compact PUCT core from the supplied MCTS; 19x19 and GoMatch scoring.
namespace {
namespace HardMCTS {
constexpr int Size = 19, Points = 361, Pass = Points, Stride = 21, Padded = 441;
static_assert(Size == GoMatch::Size && Points == GoMatch::Count, "HardAI board contract mismatch");
constexpr uint32_t None = std::numeric_limits<uint32_t>::max();
using Clock = std::chrono::steady_clock;
using Legal = std::array<uint8_t, Points + 1>;
inline int padded(int move)
{
    return (move / Size + 1) * Stride + move % Size + 1;
}
inline int unpadded(int p)
{
    return (p / Stride - 1) * Size + p % Stride - 1;
}
constexpr std::array<int, 4> Directions{-Stride, Stride, -1, 1};
inline uint64_t mix64(uint64_t x)
{
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
inline uint64_t stoneKey(int p, int color)
{
    return mix64(uint64_t(p * 3 + color));
}

struct Groups
{
    std::array<int16_t, Padded> id{};
    std::array<int16_t, Points> liberties{}, sizes{};
    int count = 0;
};

struct Board
{
    // 0 empty, 1 black, 2 white, 3 sentinel. Public for inspection only;
    // use setStone()/play() to keep the incremental hash valid.
    std::array<uint8_t, Padded> cells{};
    int turn = 1, ko = -1, passes = 0;
    double komi = 6.5;
    uint64_t stonesHash = 0;
    std::array<int, 2> capturedStones{};
    int moveCount = 0;
    std::shared_ptr<const HardAI::History> history;

    Board()
    {
        cells.fill(3);
        for (int m = 0; m < Points; ++m)
            cells[padded(m)] = 0;
    }
    void setStone(int row, int col, int color)
    {
        if (row < 0 || row >= Size || col < 0 || col >= Size || color < 0 || color > 2)
            throw std::invalid_argument("Invalid setup stone");
        int p = padded(row * Size + col);
        if (cells[p])
            stonesHash ^= stoneKey(p, cells[p]);
        cells[p] = uint8_t(color);
        if (color)
            stonesHash ^= stoneKey(p, color);
        ko = -1;
        passes = 0;
    }
    bool terminal() const { return passes >= 2; }
    uint64_t key() const
    {
        uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(komi), "double must be 64-bit");
        std::memcpy(&bits, &komi, sizeof(bits));
        return (history ? history->hash : 0) ^ stonesHash
               ^ mix64(uint64_t(turn + 4 * (ko + 1) + 2048 * passes) ^ 0x243f6a8885a308d3ULL)
               ^ mix64(bits ^ 0x13198a2e03707344ULL)
               ^ mix64((uint64_t(capturedStones[0]) + (uint64_t(capturedStones[1]) << 32))
                       ^ 0xa4093822299f31d0ULL)
               ^ mix64(uint64_t(moveCount) ^ 0x082efa98ec4e6c89ULL);
    }
    bool same(const Board &b) const
    {
        auto aHist = history, bHist = b.history;
        while (aHist != bHist) {
            if (!aHist || !bHist || aHist->hash != bHist->hash || aHist->point != bHist->point
                || aHist->player != bHist->player)
                return false;
            if (aHist->setup != bHist->setup
                && (!aHist->setup || !bHist->setup
                    || aHist->setup->initialCells != bHist->setup->initialCells
                    || aHist->setup->initialPlayer != bHist->setup->initialPlayer
                    || aHist->setup->initialKo != bHist->setup->initialKo
                    || aHist->setup->initialPasses != bHist->setup->initialPasses
                    || aHist->setup->initialMoveCount != bHist->setup->initialMoveCount
                    || aHist->setup->initialCaptured != bHist->setup->initialCaptured))
                return false;
            aHist = aHist->parent;
            bHist = bHist->parent;
        }
        return turn == b.turn && ko == b.ko && passes == b.passes && komi == b.komi
               && cells == b.cells && capturedStones == b.capturedStones
               && moveCount == b.moveCount;
    }
    Groups groups() const
    {
        Groups g;
        g.id.fill(-1);
        std::array<int16_t, Padded> marked;
        marked.fill(-1);
        std::array<int16_t, Points> queue{};
        for (int m = 0; m < Points; ++m) {
            int p = padded(m);
            if (!cells[p] || g.id[p] >= 0)
                continue;
            int id = g.count++, begin = 0, end = 1;
            queue[0] = int16_t(p);
            g.id[p] = int16_t(id);
            while (begin < end) {
                int q = queue[begin++];
                ++g.sizes[id];
                for (int d : Directions) {
                    int n = q + d;
                    if (!cells[n]) {
                        if (marked[n] != id) {
                            marked[n] = int16_t(id);
                            ++g.liberties[id];
                        }
                    } else if (cells[n] == cells[p] && g.id[n] < 0) {
                        g.id[n] = int16_t(id);
                        queue[end++] = int16_t(n);
                    }
                }
            }
        }
        return g;
    }
    Legal legalMoves(const Groups &g) const
    {
        Legal result{};
        if (terminal())
            return result;
        result[Pass] = 1;
        for (int m = 0; m < Points; ++m) {
            int p = padded(m);
            if (cells[p] || p == ko)
                continue;
            for (int d : Directions) {
                int q = p + d, color = cells[q];
                if (!color || (color == turn && g.liberties[g.id[q]] > 1)
                    || (color == 3 - turn && g.liberties[g.id[q]] == 1)) {
                    result[m] = 1;
                    break;
                }
            }
        }
        return result;
    }
    Legal legalMoves() const { return legalMoves(groups()); }
    bool legal(int move) const
    {
        if (move < 0 || move > Pass || terminal())
            return false;
        if (move == Pass)
            return true;
        return legalMoves()[move] != 0;
    }
    // Returns at first liberty; used for capture checks on the hot path.
    bool hasLiberty(int p) const
    {
        std::array<uint8_t, Padded> seen{};
        std::array<int16_t, Points> queue{};
        int begin = 0, end = 1;
        queue[0] = int16_t(p);
        seen[p] = 1;
        while (begin < end) {
            int q = queue[begin++];
            for (int d : Directions) {
                int n = q + d;
                if (!cells[n])
                    return true;
                if (cells[n] == cells[p] && !seen[n]) {
                    seen[n] = 1;
                    queue[end++] = int16_t(n);
                }
            }
        }
        return false;
    }
    // Caller MUST have checked legality, normally once when the node was expanded.
    void playKnownLegal(int move)
    {
        if (history) {
            auto next = std::make_shared<HardAI::History>();
            next->parent = history;
            next->setup = history->setup;
            next->point = move;
            next->player = turn - 1;
            next->hash = mix64(history->hash ^ uint64_t((move + 1) * 3 + turn));
            history = std::move(next);
        }
        ++moveCount;
        if (move == Pass) {
            ++passes;
            ko = -1;
            turn = 3 - turn;
            return;
        }
        int p = padded(move), captured = 0, capturedPoint = -1;
        cells[p] = uint8_t(turn);
        stonesHash ^= stoneKey(p, turn);
        std::array<int16_t, Points> queue{};
        std::array<uint8_t, Padded> seen{};
        for (int d : Directions) {
            int q = p + d;
            if (cells[q] != 3 - turn || hasLiberty(q))
                continue;
            int begin = 0, end = 1;
            queue[0] = int16_t(q);
            seen[q] = 1;
            while (begin < end) {
                int s = queue[begin++];
                for (int delta : Directions) {
                    int n = s + delta;
                    if (cells[n] == 3 - turn && !seen[n]) {
                        seen[n] = 1;
                        queue[end++] = int16_t(n);
                    }
                }
            }
            for (int i = 0; i < end; ++i) {
                int s = queue[i];
                stonesHash ^= stoneKey(s, cells[s]);
                cells[s] = 0;
                capturedPoint = s;
                ++captured;
            }
        }
        capturedStones[turn - 1] += captured;
        ko = -1;
        if (captured == 1) {
            bool isolated = true;
            int liberties = 0;
            for (int d : Directions) {
                isolated = isolated && cells[p + d] != turn;
                liberties += cells[p + d] == 0;
            }
            if (isolated && liberties == 1)
                ko = capturedPoint;
        }
        passes = 0;
        turn = 3 - turn;
    }
    bool play(int move)
    {
        if (!legal(move))
            return false;
        playKnownLegal(move);
        return true;
    }
    // Match GoMatch final territory + prisoners scoring (no dead-stone adjudication).
    double finalScore() const
    {
        int score = capturedStones[0] - capturedStones[1];
        std::array<uint8_t, Padded> seen{};
        std::array<int16_t, Points> queue{};
        for (int m = 0; m < Points; ++m) {
            int p = padded(m);
            if (cells[p])
                continue;
            if (seen[p])
                continue;
            int begin = 0, end = 1, colors = 0;
            queue[0] = int16_t(p);
            seen[p] = 1;
            while (begin < end) {
                int s = queue[begin++];
                for (int d : Directions) {
                    int n = s + d;
                    if (!cells[n] && !seen[n]) {
                        seen[n] = 1;
                        queue[end++] = int16_t(n);
                    } else if (cells[n] == 1)
                        colors |= 1;
                    else if (cells[n] == 2)
                        colors |= 2;
                }
            }
            if (colors == 1)
                score += end;
            if (colors == 2)
                score -= end;
        }
        return score - komi;
    }
    float terminalValue() const
    {
        double score = finalScore() * (turn == 1 ? 1 : -1);
        return score > 0 ? 1.f : score < 0 ? -1.f : 0.f;
    }
};

struct Evaluation
{
    std::array<float, Points + 1> logits{};
    float value = 0;
};
struct Evaluator
{
    virtual ~Evaluator() = default;
    // Value MUST be from the perspective of board.turn, not always black.
    virtual Evaluation evaluate(const Board &, const Legal &, const Groups &) = 0;
};

struct HeuristicEvaluator final : Evaluator
{
    Evaluation evaluate(const Board &b, const Legal &legal, const Groups &g) override
    {
        Evaluation result;
        int stones = 0;
        for (int m = 0; m < Points; ++m)
            stones += b.cells[padded(m)] != 0;
        result.logits.fill(-12.f);
        for (int m = 0; m < Points; ++m) {
            if (!legal[m])
                continue;
            int p = padded(m), row = m / Size, col = m % Size;
            float prior = -0.03f * float(std::abs(row - 9) + std::abs(col - 9));
            int captures = 0, saves = 0, friends = 0, empty = 0;
            std::array<int, 4> seen{-1, -1, -1, -1};
            int count = 0;
            for (int d : Directions) {
                int n = p + d;
                empty += b.cells[n] == 0;
                friends += b.cells[n] == b.turn;
                int id = g.id[n];
                if (id < 0
                    || std::find(seen.begin(), seen.begin() + count, id) != seen.begin() + count)
                    continue;
                seen[count++] = id;
                if (g.liberties[id] == 1) {
                    if (b.cells[n] == b.turn)
                        saves += g.sizes[id];
                    else
                        captures += g.sizes[id];
                }
            }
            prior += 1.4f * std::log1p(float(captures)) + 0.9f * std::log1p(float(saves));
            if (friends + empty == 4 && empty == 0 && !captures)
                prior -= 5.f;
            if (row == 0 || row == 18 || col == 0 || col == 18)
                prior -= stones < 80 ? 1.2f : 0.1f;
            if (stones < 30) {
                int edge = std::min({row, col, 18 - row, 18 - col});
                prior += edge >= 2 && edge <= 4 ? 0.8f : 0.f;
            }
            result.logits[m] = prior;
        }
        // Uncalibrated local influence heuristic for unfinished positions.
        double score = b.capturedStones[0] - b.capturedStones[1] - b.komi;
        for (int m = 0; m < Points; ++m) {
            int p = padded(m);
            if (b.cells[p])
                continue;
            int black = 0, white = 0;
            for (int delta : Directions) {
                black += b.cells[p + delta] == 1;
                white += b.cells[p + delta] == 2;
            }
            if (black && !white)
                score += black * 0.15;
            if (white && !black)
                score -= white * 0.15;
        }
        result.value = float(std::tanh(score * (b.turn == 1 ? 1 : -1) / 20.0));
        result.logits[Pass] = stones > 180 && b.passes && result.value > 0.1f ? 1.f : -8.f;
        return result;
    }
};

inline std::array<float, 5 * Points> inputPlanes(const Board &b, const Legal &legal)
{
    std::array<float, 5 * Points> planes{};
    for (int m = 0; m < Points; ++m) {
        int color = b.cells[padded(m)];
        planes[m] = float(color == b.turn);
        planes[Points + m] = float(color == 3 - b.turn);
        planes[2 * Points + m] = float(legal[m]);
        planes[3 * Points + m] = float(b.turn == 1);
        planes[4 * Points + m] = float(b.komi * (b.turn == 2 ? 1 : -1) / 20.0);
    }
    return planes;
}

struct SearchOptions
{
    int timeMs = 1800, maxDepth = 256, searchThreads = 1;
    uint32_t maxSimulations = 100000;
    size_t memoryMB = 64;
    float cpuct = 1.5f, fpuReduction = 0.2f;
};
struct SearchResult
{
    int move = Pass, completedDepth = 0;
    uint32_t simulations = 0, rootVisits = 0, reusedVisits = 0;
    double elapsedMs = 0;
    float rootValue = 0;
    bool cancelled = false, memoryLimited = false;
    uint64_t evaluations = 0, cacheHits = 0;
    size_t allocatedBytes = 0;
};

class Engine
{
#ifdef GOGAME_HARD_TESTS
    friend struct EngineProbe;
#endif
    struct Node
    {
        uint32_t first = None, visits = 0;
        uint16_t count = 0, inFlight = 0;
        bool pending = false;
        float value = 0, sum = 0;
    };
    struct Edge
    {
        uint32_t child = None;
        uint16_t prior = 0, move = Pass;
    };
    static_assert(sizeof(Edge) == 8, "Compact edge layout must remain 8 bytes");
    struct CacheEntry
    {
        uint64_t hash = 0;
        Board position{}; // Exact verification, including ko, passes and komi.
        std::array<float, Points + 1> priors{};
        float value = 0;
        bool valid = false;
    };
    Evaluator &evaluator_;
    SearchOptions options_;
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::vector<CacheEntry> cache_;
    size_t maxNodes_ = 0, maxEdges_ = 0;
    uint32_t root_ = 0;
    Board board_{};
    bool initialized_ = false, memoryLimited_ = false;
    uint64_t evaluations_ = 0, hits_ = 0;
    double latencyMs_ = 0;
    // All dynamic search workspace is reserved once and charged to memoryMB.
    std::vector<uint32_t> path, parents, remap, order;

    size_t allocatedBytes() const
    {
        return nodes_.capacity() * sizeof(Node) + edges_.capacity() * sizeof(Edge)
               + cache_.capacity() * sizeof(CacheEntry)
               + (path.capacity() + parents.capacity() + remap.capacity() + order.capacity())
                     * sizeof(uint32_t);
    }
    // Reclaim discarded siblings in-place. Child indices are allocated after
    // their parents, so reachability and node relocation need no recursive stack.
    void compact()
    {
        if (!initialized_ || root_ == 0)
            return;
        remap.assign(nodes_.size(), None);
        order.clear();
        remap[root_] = 0;
        uint32_t count = 0;
        for (uint32_t i = root_; i < nodes_.size(); ++i) {
            if (remap[i] == None)
                continue;
            remap[i] = count++;
            const auto &node = nodes_[i];
            if (node.first == None)
                continue;
            order.push_back(i);
            for (uint32_t j = node.first; j < node.first + node.count; ++j)
                if (edges_[j].child != None)
                    remap[edges_[j].child] = 0;
        }
        std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
            return nodes_[a].first < nodes_[b].first;
        });
        uint32_t first = 0;
        for (auto i : order) {
            auto &node = nodes_[i];
            const auto oldFirst = node.first;
            node.first = first;
            for (uint32_t j = oldFirst; j < oldFirst + node.count; ++j) {
                auto e = edges_[j];
                if (e.child != None)
                    e.child = remap[e.child];
                edges_[first++] = e;
            }
        }
        for (uint32_t i = root_; i < nodes_.size(); ++i)
            if (remap[i] != None)
                nodes_[remap[i]] = nodes_[i];
        nodes_.resize(count);
        edges_.resize(first);
        root_ = 0;
        memoryLimited_ = false;
    }

    uint32_t effectiveVisits(const Edge &e) const
    {
        return e.child == None ? 0 : nodes_[e.child].visits + nodes_[e.child].inFlight;
    }
    uint32_t visits(const Edge &e) const { return e.child == None ? 0 : nodes_[e.child].visits; }
    float prior(const Edge &e) const { return float(e.prior) / 65535.f; }
    // Child node sums are in the CHILD's perspective, hence the minus sign.
    float qValue(const Edge &e) const
    {
        return -nodes_[e.child].sum / float(nodes_[e.child].visits);
    }

    void resetTree(const Board &b)
    {
        nodes_.clear();
        edges_.clear();
        nodes_.push_back(Node{});
        root_ = 0;
        board_ = b;
        initialized_ = true;
        memoryLimited_ = false;
    }
    CacheEntry &evaluate(const Board &b,
                         const Legal &legal,
                         const Groups &g,
                         const std::atomic_bool *cancel = nullptr,
                         std::unique_lock<std::mutex> *treeLock = nullptr)
    {
        uint64_t key = b.key();
        auto &cached = cache_[key % cache_.size()];
        if (cached.valid && cached.hash == key && cached.position.same(b)) {
            ++hits_;
            return cached;
        }
        auto began = Clock::now();
        Evaluation evaluation;
        if (treeLock)
            treeLock->unlock();
        try {
            evaluation = evaluator_.evaluate(b, legal, g);
        } catch (...) {
            if (treeLock)
                treeLock->lock();
            throw;
        }
        if (treeLock)
            treeLock->lock();
        if (cancel && cancel->load(std::memory_order_relaxed))
            throw std::runtime_error("HardAI evaluation cancelled");
        double ms = std::chrono::duration<double, std::milli>(Clock::now() - began).count();
        latencyMs_ = std::max(ms, latencyMs_ * 0.95);
        ++evaluations_;
        // Do not publish partially filled entries if a model throws/cancels.
        cached.valid = false;
        cached.hash = key;
        cached.position = b;
        cached.value = std::isfinite(evaluation.value) ? std::clamp(evaluation.value, -1.f, 1.f)
                                                       : 0.f;
        float maximum = -std::numeric_limits<float>::infinity();
        for (int m = 0; m <= Pass; ++m)
            if (legal[m] && std::isfinite(evaluation.logits[m]))
                maximum = std::max(maximum, evaluation.logits[m]);
        double sum = 0;
        for (int m = 0; m <= Pass; ++m) {
            float weight = 0;
            if (legal[m]) {
                weight = std::isfinite(maximum)
                             ? (std::isfinite(evaluation.logits[m])
                                    ? std::exp(std::max(-80.f, evaluation.logits[m] - maximum))
                                    : 0.f)
                             : 1.f;
                // Tiny floor keeps every legal action searchable, including pass.
                weight = std::max(weight, 1e-8f);
            }
            cached.priors[m] = weight;
            sum += weight;
        }
        if (sum > 0)
            for (float &p : cached.priors)
                p = float(p / sum);
        cached.valid = true;
        return cached;
    }
    float expand(uint32_t index,
                 const Board &b,
                 const std::atomic_bool *cancel = nullptr,
                 std::unique_lock<std::mutex> *treeLock = nullptr)
    {
        if (b.terminal())
            return b.terminalValue();
        const auto g = b.groups();
        auto legal = b.legalMoves(g);
        auto &evaluation = evaluate(b, legal, g, cancel, treeLock);
        nodes_[index].value = evaluation.value;
        std::array<uint16_t, Points + 1> moves{};
        int count = 0;
        for (int m = 0; m <= Pass; ++m)
            if (legal[m])
                moves[count++] = uint16_t(m);
        if (edges_.size() + size_t(count) > maxEdges_) {
            memoryLimited_ = true;
            return evaluation.value;
        }
        std::sort(moves.begin(), moves.begin() + count, [&](int a, int c) {
            return evaluation.priors[a] != evaluation.priors[c]
                       ? evaluation.priors[a] > evaluation.priors[c]
                       : a < c;
        });
        nodes_[index].first = uint32_t(edges_.size());
        nodes_[index].count = uint16_t(count);
        for (int i = 0; i < count; ++i) {
            Edge e;
            e.move = moves[i];
            e.prior = uint16_t(
                std::clamp(std::lround(evaluation.priors[e.move] * 65535.f), 1L, 65535L));
            edges_.push_back(e);
        }
        return evaluation.value;
    }
    uint32_t select(uint32_t index) const
    {
        const Node &node = nodes_[index];
        float visitedPrior = 0;
        for (uint32_t i = node.first; i < node.first + node.count; ++i)
            if (visits(edges_[i]))
                visitedPrior += prior(edges_[i]);
        float fpu = std::max(-1.f, node.value - options_.fpuReduction * std::sqrt(visitedPrior));
        float exploration = (options_.cpuct + std::log1p(float(node.visits) / 19652.f))
                            * std::sqrt(float(std::max(1u, node.visits + node.inFlight)));
        uint32_t best = None;
        float score = -std::numeric_limits<float>::infinity();
        for (uint32_t i = node.first; i < node.first + node.count; ++i) {
            const Edge &e = edges_[i];
            if (e.child != None && nodes_[e.child].pending)
                continue;
            auto n = effectiveVisits(e);
            float q = visits(e) ? qValue(e) : fpu;
            if (e.child != None && nodes_[e.child].inFlight)
                q = (q * float(visits(e)) - float(nodes_[e.child].inFlight)) / float(n);
            float s = q + exploration * prior(e) / (1.f + float(n));
            if (s > score) {
                score = s;
                best = i;
            }
        }
        return best;
    }
    void searchParallel(const Board &b,
                        SearchResult &result,
                        const std::atomic_bool *cancel,
                        Clock::time_point deadline)
    {
        std::mutex treeMutex;
        std::exception_ptr failure;
        std::atomic_bool abort{false};
        uint32_t startedSimulations = 0;
        auto worker = [&] {
            std::vector<uint32_t> localParents;
            localParents.reserve(options_.maxDepth);
            while (!abort.load(std::memory_order_relaxed)) {
                std::unique_lock<std::mutex> lock(treeMutex);
                auto stop = [&] {
                    const double margin = std::max(1., latencyMs_ * 1.2);
                    return (cancel && cancel->load(std::memory_order_relaxed))
                           || Clock::now() + std::chrono::duration<double, std::milli>(margin)
                                  >= deadline;
                };
                if (stop() || memoryLimited_ || startedSimulations >= options_.maxSimulations)
                    break;
                Board position = b;
                uint32_t node = root_;
                float value = 0;
                localParents.clear();
                bool busy = false;
                for (int depth = 0;; ++depth) {
                    result.completedDepth = std::max(result.completedDepth, depth);
                    if (position.terminal()) {
                        value = position.terminalValue();
                        break;
                    }
                    if (nodes_[node].pending) {
                        busy = true;
                        break;
                    }
                    if (depth >= options_.maxDepth || nodes_[node].first == None)
                        break;
                    const uint32_t edge = select(node);
                    if (edge == None) {
                        busy = true;
                        break;
                    }
                    if (edges_[edge].child == None) {
                        if (nodes_.size() >= maxNodes_) {
                            memoryLimited_ = true;
                            busy = true;
                            break;
                        }
                        edges_[edge].child = uint32_t(nodes_.size());
                        nodes_.push_back(Node{});
                    }
                    localParents.push_back(node);
                    position.playKnownLegal(edges_[edge].move);
                    node = edges_[edge].child;
                }
                if (busy) {
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    continue;
                }
                if (stop())
                    break;
                ++startedSimulations;
                for (auto parent : localParents)
                    ++nodes_[parent].inFlight;
                ++nodes_[node].inFlight;
                const bool needsEvaluation = !position.terminal() && nodes_[node].first == None;
                if (needsEvaluation)
                    nodes_[node].pending = true;
                try {
                    if (needsEvaluation) {
                        if (localParents.size() >= size_t(options_.maxDepth)) {
                            const auto g = position.groups();
                            value = evaluate(position, position.legalMoves(g), g, cancel, &lock)
                                        .value;
                            nodes_[node].value = value;
                        } else
                            value = expand(node, position, cancel, &lock);
                    } else if (!position.terminal())
                        value = nodes_[node].value;
                    nodes_[node].pending = false;
                    --nodes_[node].inFlight;
                    ++nodes_[node].visits;
                    nodes_[node].sum += value;
                    for (size_t i = localParents.size(); i > 0; --i) {
                        value = -value;
                        auto &parent = nodes_[localParents[i - 1]];
                        --parent.inFlight;
                        ++parent.visits;
                        parent.sum += value;
                    }
                    ++result.simulations;
                } catch (...) {
                    nodes_[node].pending = false;
                    --nodes_[node].inFlight;
                    for (auto parent : localParents)
                        --nodes_[parent].inFlight;
                    if (!failure)
                        failure = std::current_exception();
                    abort.store(true, std::memory_order_relaxed);
                }
            }
        };
        std::vector<std::thread> workers;
        try {
            for (int i = 1; i < options_.searchThreads; ++i)
                workers.emplace_back(worker);
            worker();
        } catch (...) {
            abort.store(true);
            for (auto &t : workers)
                t.join();
            throw;
        }
        for (auto &t : workers)
            t.join();
        if (failure)
            std::rethrow_exception(failure);
    }

public:
    explicit Engine(Evaluator &evaluator, SearchOptions options = {})
        : evaluator_(evaluator)
        , options_(options)
    {
        if (options.searchThreads < 1 || options.searchThreads > 16 || options.timeMs < 0
            || options.maxDepth < 1 || options.maxDepth > 4096 || options.memoryMB < 4
            || options.memoryMB > 4096 || !std::isfinite(options.cpuct) || options.cpuct <= 0
            || !std::isfinite(options.fpuReduction) || options.fpuReduction < 0)
            throw std::invalid_argument("Invalid search options");
        const size_t bytes = options.memoryMB * size_t(1024 * 1024);
        // Leave headroom for scalar objects and the fixed Board/Groups work arrays.
        const size_t workspace = size_t(options.maxDepth) * 2 * sizeof(uint32_t)
                                     * size_t(options.searchThreads + 1)
                                 + 64 * 1024;
        cache_.resize(std::max(size_t(1), (bytes - workspace) / 4 / sizeof(CacheEntry)));
        const size_t treeBytes = bytes - workspace - cache_.capacity() * sizeof(CacheEntry);
        // remap/order permit compaction without another arena or a RAM spike.
        maxNodes_ = treeBytes / 8 / (sizeof(Node) + 2 * sizeof(uint32_t));
        maxEdges_ = (treeBytes - maxNodes_ * (sizeof(Node) + 2 * sizeof(uint32_t))) / sizeof(Edge);
        nodes_.reserve(maxNodes_);
        edges_.reserve(maxEdges_);
        remap.reserve(maxNodes_);
        order.reserve(maxNodes_);
        path.reserve(size_t(options.maxDepth));
        parents.reserve(size_t(options.maxDepth));
        if (allocatedBytes() + 64 * 1024 > bytes)
            throw std::runtime_error("Arena allocator exceeded HardAI memory budget");
    }
    Board rootBoard() const { return board_; }
    void clear()
    {
        initialized_ = false;
        nodes_.clear();
        edges_.clear();
        for (auto &entry : cache_)
            entry.valid = false;
    }
    bool advance(int move)
    {
        if (!initialized_ || !board_.legal(move))
            return false;
        uint32_t child = None;
        const Node &n = nodes_[root_];
        if (n.first != None)
            for (uint32_t i = n.first; i < n.first + n.count; ++i)
                if (edges_[i].move == move) {
                    child = edges_[i].child;
                    break;
                }
        board_.playKnownLegal(move);
        if (child == None)
            resetTree(board_);
        else {
            root_ = child;
            if (memoryLimited_ || edges_.size() > maxEdges_ * 3 / 4
                || nodes_.size() > maxNodes_ * 3 / 4)
                compact();
        }
        return true;
    }
    SearchResult search(const Board &b,
                        const std::atomic_bool *cancel = nullptr,
                        Clock::time_point deadline = Clock::time_point::max())
    {
        const auto started = Clock::now();
        deadline = std::min(deadline, started + std::chrono::milliseconds(options_.timeMs));
        auto cancelled = [&] { return cancel && cancel->load(std::memory_order_relaxed); };
        auto stop = [&] {
            double margin = std::max(1.0, latencyMs_ * 1.2);
            return cancelled()
                   || Clock::now() + std::chrono::duration<double, std::milli>(margin) >= deadline;
        };
        if (!initialized_ || !board_.same(b))
            resetTree(b);
        else if (root_ != 0
                 && (memoryLimited_ || edges_.size() > maxEdges_ * 3 / 4
                     || nodes_.size() > maxNodes_ * 3 / 4))
            compact();
        SearchResult result;
        result.reusedVisits = nodes_[root_].visits;
        auto startEvals = evaluations_, startHits = hits_;
        if (!b.terminal()) {
            // Safe legal fallback even for zero budget / cancellation before search.
            auto legal = b.legalMoves();
            for (int m = 0; m <= Pass; ++m)
                if (legal[m]) {
                    result.move = m;
                    break;
                }
            if (!stop() && nodes_[root_].first == None)
                expand(root_, b, cancel);
            if (options_.searchThreads > 1)
                searchParallel(b, result, cancel, deadline);
            else
                while (!stop() && !memoryLimited_ && result.simulations < options_.maxSimulations) {
                    Board position = b;
                    uint32_t node = root_;
                    float value = 0;
                    path.clear();
                    parents.clear();
                    bool incomplete = false;
                    for (int depth = 0;; ++depth) {
                        result.completedDepth = std::max(result.completedDepth, depth);
                        if (cancelled()) {
                            incomplete = true;
                            break;
                        }
                        if (position.terminal()) {
                            value = position.terminalValue();
                            break;
                        }
                        if (depth >= options_.maxDepth) {
                            if (nodes_[node].first == None) {
                                if (stop()) {
                                    incomplete = true;
                                    break;
                                }
                                const auto g = position.groups();
                                value = evaluate(position, position.legalMoves(g), g, cancel).value;
                                nodes_[node].value = value;
                            } else
                                value = nodes_[node].value;
                            break;
                        }
                        if (nodes_[node].first == None) {
                            if (stop()) {
                                incomplete = true;
                                break;
                            }
                            value = expand(node, position, cancel);
                            break;
                        }
                        uint32_t edge = select(node);
                        if (edges_[edge].child == None) {
                            if (nodes_.size() >= maxNodes_) {
                                memoryLimited_ = true;
                                incomplete = true;
                                break;
                            }
                            edges_[edge].child = uint32_t(nodes_.size());
                            nodes_.push_back(Node{});
                        }
                        parents.push_back(node);
                        path.push_back(edge);
                        position.playKnownLegal(edges_[edge].move);
                        node = edges_[edge].child;
                        if ((depth & 7) == 7 && stop()) {
                            incomplete = true;
                            break;
                        }
                    }
                    if (incomplete)
                        break;
                    ++nodes_[node].visits;
                    nodes_[node].sum += value;
                    for (size_t i = path.size(); i > 0; --i) {
                        value = -value;
                        Node &parent = nodes_[parents[i - 1]];
                        ++parent.visits;
                        parent.sum += value;
                    }
                    ++result.simulations;
                }
            const Node &root = nodes_[root_];
            if (root.first != None && root.count) {
                uint32_t best = root.first;
                for (uint32_t i = root.first + 1; i < root.first + root.count; ++i) {
                    const auto &e = edges_[i];
                    const auto &v = edges_[best];
                    if (visits(e) > visits(v) || (visits(e) == visits(v) && e.prior > v.prior))
                        best = i;
                }
                result.move = edges_[best].move;
                result.rootValue = visits(edges_[best]) ? qValue(edges_[best]) : root.value;
            }
        }
        result.rootVisits = nodes_[root_].visits;
        result.evaluations = evaluations_ - startEvals;
        result.cacheHits = hits_ - startHits;
        result.cancelled = cancelled();
        result.memoryLimited = memoryLimited_;
        result.allocatedBytes = allocatedBytes();
        result.elapsedMs = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
        return result;
    }
};

} // namespace HardMCTS
} // namespace

struct HardAI::Impl
{
    struct Adapter final : HardMCTS::Evaluator
    {
        std::shared_ptr<HardAI::Evaluator> model;
        const std::atomic_bool *cancel = nullptr;
        HardMCTS::HeuristicEvaluator heuristic;
        HardMCTS::Evaluation evaluate(const HardMCTS::Board &b,
                                      const HardMCTS::Legal &legal,
                                      const HardMCTS::Groups &g) override
        {
            if (!model)
                return heuristic.evaluate(b, legal, g);
            HardAI::Position p;
            for (int m = 0; m < GoMatch::Count; ++m)
                p.cells[m] = int(b.cells[HardMCTS::padded(m)]) - 1;
            p.currentPlayer = b.turn - 1;
            p.koPoint = b.ko < 0 ? -1 : HardMCTS::unpadded(b.ko);
            p.consecutivePasses = b.passes;
            p.komi = b.komi;
            p.capturedStones = b.capturedStones;
            p.moveCount = b.moveCount;
            p.history = b.history;
            auto result = model->evaluate(p, legal, cancel);
            return {result.policyLogits, result.value};
        }
    } adapter;
    std::mutex mutex;
    std::unique_ptr<HardMCTS::Engine> engine;
};

HardAI::HardAI(HardSearchOptions options)
    : m_options(options)
    , m_impl(std::make_shared<Impl>())
{
    if (options.searchThreads < 1 || options.searchThreads > 16 || options.timeBudgetMs < 0
        || options.memoryMB < 4 || options.memoryMB > 4096 || options.maxDepth < 1
        || options.maxDepth > 4096 || !std::isfinite(options.cpuct) || options.cpuct <= 0
        || !std::isfinite(options.fpuReduction) || options.fpuReduction < 0)
        throw std::invalid_argument("Invalid HardAI search options");
}
void HardAI::setEvaluator(std::shared_ptr<Evaluator> evaluator)
{
    auto next = std::make_shared<Impl>();
    next->adapter.model = std::move(evaluator);
    m_impl = std::move(next);
}
void HardAI::reset()
{
    setEvaluator(m_impl->adapter.model);
}
QString HardAI::backendName() const
{
    return m_impl->adapter.model ? m_impl->adapter.model->name() : QStringLiteral("PUCT heuristic");
}
std::array<float, 5 * GoMatch::Count> HardAI::inputPlanes(const Position &p, const LegalMask &legal)
{
    std::array<float, 5 * GoMatch::Count> planes{};
    for (int m = 0; m < GoMatch::Count; ++m) {
        planes[m] = float(p.cells[m] == p.currentPlayer);
        planes[GoMatch::Count + m] = float(p.cells[m] == 1 - p.currentPlayer);
        planes[2 * GoMatch::Count + m] = float(legal[m]);
        planes[3 * GoMatch::Count + m] = float(p.currentPlayer == 0);
        planes[4 * GoMatch::Count + m] = float(p.komi * (p.currentPlayer == 1 ? 1 : -1) / 20.0);
    }
    return planes;
}
SearchResult HardAI::chooseMove(const GoMatch &current,
                                const std::shared_ptr<std::atomic_bool> &token) const
{
    const auto started = std::chrono::steady_clock::now();
    const auto deadline = started + std::chrono::milliseconds(m_options.timeBudgetMs);
    SearchResult out;
    const auto finish = [&] {
        out.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - started)
                            .count();
        return out;
    };
    const auto cancelled = [&] { return token && token->load(std::memory_order_relaxed); };
    if (cancelled()) {
        out.cancelled = true;
        return finish();
    }
    if (current.m_passes >= 2) {
        out.stopReason = QStringLiteral("Game ended");
        return finish();
    }
    auto state = m_impl; // Keep this backend/cache alive if the UI configures a new one.
    std::unique_lock<std::mutex> lock(state->mutex, std::defer_lock);
    while (!lock.try_lock()) {
        if (cancelled()) {
            out.cancelled = true;
            return finish();
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            out.cancelled = true;
            out.stopReason = QStringLiteral("HardAI busy");
            return finish();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    try {
        HardMCTS::Board board;
        for (int m = 0; m < GoMatch::Count; ++m)
            if (current.cells()[m] >= 0)
                board.setStone(m / GoMatch::Size, m % GoMatch::Size, current.cells()[m] + 1);
        board.turn = current.currentPlayer + 1;
        board.ko = current.koPoint < 0 ? -1 : HardMCTS::padded(current.koPoint);
        board.passes = current.m_passes;
        board.moveCount = current.moves;
        board.capturedStones = current.capturedStones;
        if (state->adapter.model) {
            auto setup = std::make_shared<GoMatch::Replay>(current.replay());
            const auto moves = std::move(setup->moves);
            setup->moves.clear();
            auto initial = std::make_shared<History>();
            initial->setup = setup;
            uint64_t hash = 0x6a09e667f3bcc909ULL;
            for (int c : setup->initialCells)
                hash = HardMCTS::mix64(hash ^ uint64_t(c + 2));
            hash ^= HardMCTS::mix64(uint64_t(setup->initialMoveCount));
            initial->hash = hash;
            board.history = initial;
            for (const auto &m : moves) {
                auto next = std::make_shared<History>();
                next->parent = board.history;
                next->setup = setup;
                next->player = m.player;
                next->point = m.point;
                next->hash = HardMCTS::mix64(board.history->hash
                                             ^ uint64_t((m.point + 1) * 3 + m.player + 1));
                board.history = next;
            }
        }
        if (!state->engine) {
            HardMCTS::SearchOptions options;
            options.timeMs = m_options.timeBudgetMs;
            options.searchThreads = m_options.searchThreads;
            options.memoryMB = size_t(m_options.memoryMB);
            options.maxDepth = m_options.maxDepth;
            options.maxSimulations = m_options.maxSimulations;
            options.cpuct = m_options.cpuct;
            options.fpuReduction = m_options.fpuReduction;
            state->engine = std::make_unique<HardMCTS::Engine>(state->adapter, options);
        }
        state->adapter.cancel = token.get();
        // Reuse only an exactly reproduced opponent move, including captures,
        // passes, ko and prisoner counts; undo/load are never guessed.
        const auto previous = state->engine->rootBoard();
        if (!previous.same(board) && board.moveCount == previous.moveCount + 1
            && !previous.terminal()) {
            auto next = previous;
            next.playKnownLegal(HardMCTS::Pass);
            if (next.same(board))
                state->engine->advance(HardMCTS::Pass);
            else
                for (int move = 0; move < GoMatch::Count; ++move) {
                    int p = HardMCTS::padded(move);
                    if (!previous.cells[p] && board.cells[p] == previous.turn) {
                        next = previous;
                        if (next.play(move) && next.same(board)) {
                            state->engine->advance(move);
                            break;
                        }
                    }
                }
        }
        const auto result = state->engine->search(board, token.get(), deadline);
        state->adapter.cancel = nullptr;
        out.pass = result.move == HardMCTS::Pass;
        if (!out.pass) {
            out.row = result.move / GoMatch::Size;
            out.col = result.move % GoMatch::Size;
        }
        out.cancelled = result.cancelled;
        out.nodes = int(result.simulations);
        out.completedDepth = result.completedDepth;
        out.winValue = result.rootValue;
        out.estimatedScore = board.finalScore();
        out.evaluations = result.evaluations;
        out.cacheHits = result.cacheHits;
        out.rootVisits = result.rootVisits;
        out.reusedVisits = result.reusedVisits;
        out.allocatedBytes = result.allocatedBytes;
        out.memoryLimited = result.memoryLimited;
        out.stopReason = result.memoryLimited ? QStringLiteral("Memory budget")
                                              : QStringLiteral("PUCT budget");
        out.stoppedEarly = result.memoryLimited;
        // Keep own reply as the root; the next request may supply an opponent
        // move. Incompatible/new positions are always rebuilt by search().
        if (!out.cancelled)
            state->engine->advance(result.move);
    } catch (const std::exception &e) {
        state->adapter.cancel = nullptr;
        if (cancelled())
            out.cancelled = true;
        else {
            out.error = QString::fromUtf8(e.what());
            out.stopReason = QStringLiteral("HardAI backend error");
        }
    }
    return finish();
}
