#include "MediumAI.h"
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace std;

template<class T>
T calc(T &val, T Min, T Max)
{
    if (val > Max)
        return Max;
    if (val < Min)
        return Min;
    return val;
}

MediumAI ::MediumAI(SearchOptions options)
{
    m_options = options;
    m_options.maxDepth = calc(options.maxDepth, 1, 5);
    m_options.candidateLimit = calc(options.candidateLimit, 1, 24);
    m_options.timeBudgetMs = calc(options.timeBudgetMs, 1, 2000);
}

double MediumAI ::evaluate(const GoMatch &game) const
{
    if (game.consecutivePasses >= 2)
        return game.score() * 1000;

    // const int Count = GoMatch::Count;
    // array<int, Count> &board = game.cells();
    auto &board = game.cells(); // Cần tối ưu nhưng chịu
    double val = -6.5;
    auto &groups = game.groups(); // Cần tối ưu nhưng chịu
    array<double, 361> influence{};
    for (int pos = 0; pos < 361; ++pos) {
        if (board[pos] < 0)
            continue;
        int sign = board[pos] == 0 ? 1 : -1;
        val += sign * 12;
        int root = groups.root(pos);
        if (root == pos) {
            int count = int(groups.liberties[root].count());
            val += sign
                   * (count == 1   ? -5.0 * groups.size[root]
                      : count == 2 ? -1.0 * groups.size[root]
                                   : min(count, 8) * 0.5);
            // công thức tính dù là quân mình hay quân ta
            // số khí còn lại 1 -> 5*(số quân của dsu)
            // số khí còn lại 2 -> 5*(số quân của dsu)
            // số khí còn lại <= 8 -> 0.5*(số khí) (chưa quan trọng)
            // nhiều hơn -> 0.5 * 8
            // Nếu thuật toán yếu hãy thay đổi hệ số.
        }

        for (int dr = -2; dr <= 2; ++dr) {
            for (int dc = -2; dc <= 2; ++dc) {
                int dist = abs(dr) + abs(dc);
                int r = pos / 19 + dr;
                int c = pos % 19 + dc;
                if (dist > 0 && dist <= 2 && r >= 0 && r < 19 && c >= 0 && c < 19
                    && board[r * 19 + c] == -1)
                    influence[r * 19 + c] += double(sign) / dist;
                // tính trước ảnh hưởng các ô gần pos
            }
        }
    }
    for (int pos = 0; pos < 361; ++pos) {
        if (board[pos] == -1)
            val += calc(influence[pos], -1.0, 1.0) * 0.35;
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
    };

    Search(const MediumAI &engine, const shared_ptr<atomic_bool> &token)
    {
        options = engine.m_options;
        cancel = token;
        m_engine = engine;
        timer.start();
    }

    SearchOptions options;
    shared_ptr<atomic_bool> cancel;
    QElapsedTimer timer;
    int nodes = 0;
    bool incomplete = 0;

private:
    MediumAI m_engine;

public:
    bool stopped()
    {
        if ((cancel && cancel->load()) || timer.elapsed() >= options.timeBudgetMs) {
            return incomplete = true;
        }
        return false;
    }

    vector<Candidate> candidates(const GoMatch &game)
    {
        vector<Candidate> res;
        const auto &board = game.cells();
        const int sign = game.currentPlayer == 0 ? 1 : -1;
        array<bool, 361> nxt{};
        // Các ô quan trọng nên chiếm.
        for (int r : {3, 9, 15})
            for (int c : {3, 9, 15})
                nxt[r * 19 + c] = true;

        for (int pos = 0; pos < 361; ++pos) {
            if (board[pos] >= 0)
                for (int dr = -2; dr <= 2; ++dr) {
                    for (int dc = -2; dc <= 2; ++dc) {
                        int r = pos / 19 + dr;
                        int c = pos % 19 + dc;
                        if (r >= 0 && r < 19 && c >= 0 && c < 19)
                            nxt[r * 19 + c] = 1;
                    }
                }
        }

        for (int pos = 0; pos < 361; ++pos) {
            if (stopped())
                break;
            if (board[pos] != -1 || !nxt[pos])
                continue;
            GoMatch next = game;
            QString error;
            if (!next.play(pos / 19, pos % 19, error))
                continue;
            double priority = sign * m_engine.evaluate(next);
            int r = pos / 19;
            int c = pos % 19;
            int edge = min({r, c, 18 - r, 18 - c});

            if (game.moveCount < 30) // đầu game
                priority += edge == 3 ? 1.2 : edge == 2 ? 0.6 : edge == 0 ? -1.5 : 0;
            res.push_back({pos, move(next), priority});

            if (static_cast<int>(res.size()) > options.candidateLimit) {
                auto worst = min_element(res.begin(),
                                         res.end(),
                                         [&](const Candidate &a, const Candidate &b) {
                                             return a.priorty == b.priorty ? a.pos > b.pos
                                                                           : a.priorty < b.priorty;
                                         });
                res.erase(worst);
            }
        }
        sort(res.begin(), res.end(), [&](const Candidate &a, const Candidate &b) {
            return a.priorty == b.priorty ? a.pos < b.pos : a.priorty > b.priorty;
        });
        // nếu không đủ mạnh hãy sài stable_sort
        if (static_cast<int>(res.size()) > options.candidateLimit)
            res.resize(options.candidateLimit); // giới hạn số node
        GoMatch passed = game;
        passed.pass();
        res.push_back({-1, passed, sign * m_engine.evaluate(passed)}); // Lượt sau trong node
        return res;
    }

    double minimax(const GoMatch &game,
                   int depth,
                   double alpha = numeric_limits<double>::lowest(),
                   double beta = numeric_limits<double>::max())
    {
        ++nodes;
        if (stopped() || depth == 0 || game.consecutivePasses >= 2)
            return m_engine.evaluate(game);
        double best = game.currentPlayer == 0 ? numeric_limits<double>::lowest()
                                              : numeric_limits<double>::max();

        auto moves = candidates(game);

        for (const auto &move : moves) {
            if (stopped())
                break;

            double score = minimax(move.next, depth - 1, alpha, beta);
            best = game.currentPlayer == 0 ? max(best, score) : min(best, score);
            if (!game.currentPlayer)
                alpha = max(alpha, best);
            else
                beta = min(beta, best);

            if (beta <= alpha)
                break; // tối ưu hệ số
        }

        return isfinite(best) ? best : m_engine.evaluate(game);
    }
};

SearchResult MediumAI::chooseMove(const GoMatch &cur, const shared_ptr<atomic_bool> &cancel) const
{
    Search search(*this, cancel);
    SearchResult res;

    if (cur.consecutivePasses >= 2)
        return res;

    for (int pos = 0; pos < 361; ++pos) {
        if (cancel && cancel->load()) {
            res.cancelled = true;
            return res;
        }

        auto next = cur;

        QString error;

        if (next.play(pos / 19, pos % 19, error)) {
            res.row = pos / 19;
            res.col = pos % 19;
            res.pass = false;
            break;
        }
    }

    for (int depth = 1; depth <= search.options.maxDepth; ++depth) {
        if (search.stopped())
            break;
        auto u = search.candidates(cur);
        if (search.incomplete)
            break;
        double best = cur.currentPlayer == 0 ? numeric_limits<double>::lowest()
                                             : numeric_limits<double>::max();

        int chosen = -1;
        for (const auto &v : u) {
            double score = search.minimax(v.next, depth - 1);
            if (search.incomplete)
                break;
            if ((cur.currentPlayer == 0 && score > best)
                || (cur.currentPlayer == 1 && score < best)) {
                best = score;
                chosen = v.pos;
            }
        }

        if (search.incomplete)
            break;

        res.pass = (chosen == -1);
        res.row = chosen < 0 ? -1 : chosen / 19;
        res.col = chosen < 0 ? -1 : chosen % 19;
        res.completedDepth = depth;
    }

    res.cancelled = (cancel && cancel->load());
    res.nodes = search.nodes;
    res.elapsedMs = search.timer.elapsed();
    return res;
}
