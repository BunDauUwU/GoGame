# Mã MCTS người dùng đính kèm

Đầu vào của yêu cầu HardAI ngày 19/10/2026, từ file `Pasted text.txt`. Đây là bản mã tham khảo người dùng gửi, không phải implementation hiện tại. Giữ nguyên nội dung bên trong code block; xem bản thực thi ở `src/AI/HardAI.h/.cpp` tính từ gốc dự án.

Nguồn tệp gốc: `/home/lumie/.codex/attachments/c37ac408-66fe-435f-8f62-7aea3666a011/Pasted text.txt`.

````cpp
#ifndef GOMCTS_IMPLEMENTATION_INCLUDED
#define GOMCTS_IMPLEMENTATION_INCLUDED

#include "HardAI.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#ifdef GOMCTS_ONNX
#include <onnxruntime_cxx_api.h>
#endif
using namespace std;

namespace GoMCTS {
constexpr int Size = 10, Points = 361, Pass = Points, Stride = 21, Padded = 441;
constexpr uint32_t None = numeric_limits<uint32_t>::max();

using Clock = chrono::steady_clock;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using Legal = array<u8, Points + 1>;

int padded(int move)
{
    return (move / Size + 1) * Stride + move % Size + 1;
}

int unpadded(int p)
{
    return (p / Stride - 1) * Size + p % Stride - 1;
}
constexpr array<int, 4> d{-Stride, Stride, -1, 1};
u64 mix64(u64 x)
{ // I dunno why i need SplitMix64()
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
inline u64 stoneKey(int p, int color)
{
    return mix64(u64(p * 3 + color));
}

// Build ividiual Group and Board for optimize time and data
struct Groups
{
    array<int16_t, Padded> id{};
    array<int16_t, Points> liberties{}, sizes{};
    int count = 0;
};

struct Board
{
    // 0 empty, 1 black, 2 white, 3 sentinel. Public for inspection only;
    // use setStone()/play() to keep the incremental hash valid.
    array<u8, Padded> cells{};
    int turn = 1, ko = -1, passes = 0;
    double komi = 6.5;
    u64 stonesHash = 0;

    Board()
    {
        cells.fill(3);
        for (int m = 0; m < Points; ++m)
            cells[padded(m)] = 0;
    }

    void setStone(int row, int col, int color)
    {
        if (row < 0 || row >= Size || col < 0 || col >= Size || color < 0 || color > 2)
            throw invalid_argument("Invalid setup stone"); // cancel here for gay
        int p = padded(row * Size + col);

        if (cells[p])
            stonesHash ^= stoneKey(p, cells[p]);
        cells[p] = u8(color);
        if (color)
            stonesHash ^= stoneKey(p, color);
        ko = -1;
        passes = 0;
    }

    bool terminal() const { return passes >= 2; }
    u64 key() const
    {
        u64 bits = 0;
        static_assert(sizeof(bits) == sizeof(komi), "double must be 64-bit");
        memcpy(&bits, &komi, sizeof(bits));
        return stonesHash ^ mix64(u64(turn + 4 * (ko + 1) + 2048 * passes) ^ mix64(bits));
    }
    bool same(const Board &b) const
    {
        return turn == b.turn && ko == b.ko && passes == b.passes && komi == b.komi
               && cells == b.cells;
    }

    Groups groups() const
    {
        Groups g;
        g.id.fill(-1);
        array<int16_t, Padded> marked;
        marked.fill(-1);
        array<int16_t, Points> queue{};

        for (int m = 0; m < Points; ++m) {
            int p = padded(m);
            if (!cells[p] || g.id[p] >= 0)
                continue;
            int id = g.count++, l = 0, r = 1;
            queue[0] = int16_t(p);
            g.id[p] = int16_t(id);
            while (l < r) {
                int q = queue[l++];
                ++g.sizes[id];
                for (int dist : d) {
                    int n = q + dist;
                    if (!cells[n]) {
                        if (marked[n] != id) {
                            marked[n] = int16_t(id);
                            ++g.liberties[id];
                        }
                    } else if (cells[n] == cells[p] && g.id[n] < 0) {
                        g.id[n] = int16_t(id);
                        queue[r++] = int16_t(n);
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
            for (int dist : d) {
                int q = p + dist, color = cells[q];
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

    bool hasLiberty(int p) const
    {
        array<uint8_t, Padded> seen{};
        array<int16_t, Points> queue{};
        int l = 0, r = 1;
        queue[0] = int16_t(p);
        seen[p] = 1;
        while (l < r) {
            int q = queue[l++];
            for (int dist : d) {
                int n = q + dist;
                if (!cells[n])
                    return true;
                if (cells[n] == cells[p] && !seen[n]) {
                    seen[n] = 1;
                    queue[r++] = int16_t(n);
                }
            }
        }
        return false;
    } // return at first liberty

    void playKnownLegal(int move)
    {
        if (move == Pass) {
            ++passes;
            ko = -1;
            turn = 3 - turn;
            return;
        }
        int p = padded(move), captured = 0, capturedPoint = -1;
        cells[p] = u8(turn);
        stonesHash ^= stoneKey(p, turn);
        array<int16_t, Points> queue{};
        array<u8, Padded> seen{};
        for (int dist : d) {
            int q = p + dist;
            if (cells[q] != 3 - turn || hasLiberty(q))
                continue;
            int l = 0, r = 1;
            queue[0] = int16_t(q);
            seen[q] = 1;
            while (l < r) {
                int s = queue[l++];
                for (int delta : d) {
                    int n = s + delta;
                    if (cells[n] == 3 - turn && !seen[n]) {
                        seen[n] = 1;
                        queue[r++] = int16_t(n);
                    }
                }
            }
            for (int i = 0; i < r; ++i) {
                int s = queue[i];
                stonesHash ^= stoneKey(s, cells[s]);
                cells[s] = 0;
                capturedPoint = s;
                ++captured;
            }
        }
        ko = -1;
        if (captured == 1) {
            bool isolated = true;
            int liberties = 0;
            for (int dist : d) {
                isolated = isolated && cells[p + dist] != turn;
                liberties += cells[p + dist] == 0;
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

    double areaScore() const
    {
        int score = 0;
        array<uint8_t, Padded> seen{};
        array<int16_t, Points> queue{};
        for (int m = 0; m < Points; ++m) {
            int p = padded(m);
            if (cells[p]) {
                score += cells[p] == 1 ? 1 : -1;
                continue;
            }
            if (seen[p])
                continue;
            int l = 0, r = 1, colors = 0;
            queue[0] = int16_t(p);
            seen[p] = 1;
            while (l < r) {
                int s = queue[l++];
                for (int dist : d) {
                    int n = s + dist;
                    if (!cells[n] && !seen[n]) {
                        seen[n] = 1;
                        queue[r++] = int16_t(n);
                    } else if (cells[n] == 1)
                        colors |= 1;
                    else if (cells[n] == 2)
                        colors |= 2;
                }
            }
            if (colors == 1)
                score += r;
            if (colors == 2)
                score -= r;
        }
        return score - komi;
    }

    float terminalValue() const
    {
        double score = areaScore() * (turn == 1 ? 1 : -1);
        return score > 0 ? 1.f : score < 0 ? -1.f : 0.f;
    }
};

struct Evaluation
{
    array<float, Points + 1> logits{};
    float value = 0;
};
struct Evaluator
{
    virtual ~Evaluator() = default;
    virtual Evaluation evaluate(const Board &, const Legal &) = 0;
};

struct HeuristicEvaluator final : Evaluator
{
    Evaluation evaluate(const Board &b, const Legal &legal) override
    {
        Evaluation result;
        Groups g = b.groups();
        int stones = 0;
        for (int m = 0; m < Points; ++m)
            stones += b.cells[padded(m)] != 0;
        result.logits.fill(-12.f);
        for (int m = 0; m < Points; ++m) {
            if (!legal[m])
                continue;
            int p = padded(m), row = m / Size, col = m % Size;
            float prior = -0.03f * float(abs(row - 9) + abs(col - 9));
            int captures = 0, saves = 0, friends = 0, empty = 0;
            array<int, 4> seen{-1, -1, -1, -1};
            int count = 0;
            for (int dist : d) {
                int n = p + dist;
                empty += b.cells[n] == 0;
                friends += b.cells[n] == b.turn;
                int id = g.id[n];
                if (id < 0 || find(seen.begin(), seen.begin() + count, id) != seen.begin() + count)
                    continue;
                seen[count++] = id;
                if (g.liberties[id] == 1) {
                    if (b.cells[n] == b.turn)
                        saves += g.sizes[id];
                    else
                        captures += g.sizes[id];
                }
            }
            prior += 1.4f * log1p(float(captures)) + 0.9f * log1p(float(saves));
            if (friends + empty == 4 && empty == 0 && !captures)
                prior -= 5.f;
            if (row == 0 || row == 18 || col == 0 || col == 18)
                prior -= stones < 80 ? 1.2f : 0.1f;
            if (stones < 30) {
                int edge = min({row, col, 18 - row, 18 - col});
                prior += edge >= 2 && edge <= 4 ? 0.8f : 0.f;
            }
            result.logits[m] = prior;
        }

        result.logits[Pass] = b.passes ? (b.areaScore() * (b.turn == 1 ? 1 : -1) > 0 ? 1.f : -5.f)
                                       : -5.f;
        result.value = float(tanh(b.areaScore() * (b.turn == 1 ? 1 : -1) / 25.0));
        return result;
    }
};

inline array<float, 5 * Points> inputPlanes(const Board &b, const Legal &legal)
{
    array<float, 5 * Points> planes{};
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
    int timeMs = 1800, maxDepth = 256;
    u32 maxSimulations = 100000;
    size_t memoryMB = 64;
    float cpuct = 1.5f, fpuReduction = 0.2f;
};
struct SearchResult
{
    int move = Pass;
    u32 simulations = 0, rootVisits = 0;
    double elapsedMs = 0;
    float rootValue = 0;
    bool cancelled = false, memoryLimited = false;
    u64 evaluations = 0, cacheHits = 0;
    size_t allocatedBytes = 0;
};

class Engine
{
    struct Node
    {
        u32 first = None, visits = 0;
        uint16_t count = 0;
        float value = 0, sum = 0;
    };
    struct Edge
    {
        u32 child = None;
        uint16_t prior = 0, move = Pass;
    };
    static_assert(sizeof(Edge) == 8, "Compact edge layout must remain 8 bytes");
    struct CacheEntry
    {
        u64 hash = 0;
        Board position{};
        array<float, Points + 1> priors{};
        float value = 0;
        bool valid = false;
    };
    Evaluator &evaluator;
    SearchOptions options;
    vector<Node> nodes;
    vector<Edge> edges;
    vector<CacheEntry> cache;
    size_t maxNodes = 0, maxEdges = 0;
    u32 root = 0;
    Board board{};
    bool initialized = false, memoryLimited = false;
    u64 evaluations = 0, hits = 0;
    double latencyMs = 0;

    u32 visits(const Edge &e) const { return e.child == None ? 0 : nodes[e.child].visits; }
    float prior(const Edge &e) const { return float(e.prior) / 65535.f; }
    float qValue(const Edge &e) const { return -nodes[e.child].sum / float(nodes[e.child].visits); }

    void resetTree(const Board &b)
    {
        nodes.clear();
        edges.clear();
        nodes.push_back(Node{});
        root = 0;
        board = b;
        initialized = true;
        memoryLimited = false;
    }

    CacheEntry &evaluate(const Board &b, const Legal &legal)
    {
        uint64_t key = b.key();
        auto &cached = cache[key % cache.size()];
        if (cached.valid && cached.hash == key && cached.position.same(b)) {
            ++hits;
            return cached;
        }
        auto began = Clock::now();
        Evaluation evaluation = evaluator.evaluate(b, legal);
        double ms = chrono::duration<double, milli>(Clock::now() - began).count();
        latencyMs = max(ms, latencyMs * 0.95);
        ++evaluations;
        cached.valid = true;
        cached.hash = key;
        cached.position = b;
        cached.value = isfinite(evaluation.value) ? clamp(evaluation.value, -1.f, 1.f) : 0.f;
        float maximum = -numeric_limits<float>::infinity();
        for (int m = 0; m <= Pass; ++m)
            if (legal[m] && isfinite(evaluation.logits[m]))
                maximum = max(maximum, evaluation.logits[m]);
        double sum = 0;
        for (int m = 0; m <= Pass; ++m) {
            float weight = 0;
            if (legal[m]) {
                weight = isfinite(maximum) ? (isfinite(evaluation.logits[m])
                                                  ? exp(max(-80.f, evaluation.logits[m] - maximum))
                                                  : 0.f)
                                           : 1.f;

                weight = max(weight, 1e-8f);
            }
            cached.priors[m] = weight;
            sum += weight;
        }
        if (sum > 0)
            for (float &p : cached.priors)
                p = float(p / sum);
        return cached;
    }
    float expand(uint32_t index, const Board &b)
    {
        if (b.terminal())
            return b.terminalValue();
        auto legal = b.legalMoves();
        auto &evaluation = evaluate(b, legal);
        nodes[index].value = evaluation.value;
        array<uint16_t, Points + 1> moves{};
        int count = 0;
        for (int m = 0; m <= Pass; ++m)
            if (legal[m])
                moves[count++] = uint16_t(m);
        if (edges.size() + size_t(count) > maxEdges) {
            memoryLimited = true;
            return evaluation.value;
        }
        sort(moves.begin(), moves.begin() + count, [&](int a, int c) {
            return evaluation.priors[a] != evaluation.priors[c]
                       ? evaluation.priors[a] > evaluation.priors[c]
                       : a < c;
        });
        nodes[index].first = uint32_t(edges.size());
        nodes[index].count = uint16_t(count);
        for (int i = 0; i < count; ++i) {
            Edge e;
            e.move = moves[i];
            e.prior = uint16_t(clamp(lround(evaluation.priors[e.move] * 65535.f), 1L, 65535L));
            edges.push_back(e);
        }
        return evaluation.value;
    }
    uint32_t select(uint32_t index) const
    {
        const Node &node = nodes[index];
        float visitedPrior = 0;
        for (uint32_t i = node.first; i < node.first + node.count; ++i)
            if (visits(edges[i]))
                visitedPrior += prior(edges[i]);
        float fpu = max(-1.f, node.value - options.fpuReduction * sqrt(visitedPrior));
        float exploration = (options.cpuct + log1p(float(node.visits) / 19652.f))
                            * sqrt(float(max(1u, node.visits)));
        uint32_t best = node.first;
        float score = -numeric_limits<float>::infinity();
        for (uint32_t i = node.first; i < node.first + node.count; ++i) {
            const Edge &e = edges[i];
            auto n = visits(e);
            float q = n ? qValue(e) : fpu;
            float s = q + exploration * prior(e) / (1.f + float(n));
            if (s > score) {
                score = s;
                best = i;
            }
        }
        return best;
    }
};
} // namespace GoMCTS
````
