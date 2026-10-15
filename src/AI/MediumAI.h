#pragma once
#include "../Rules/GoMatch.h"
#include <atomic>
#include <memory>

struct SearchOptions
{
    int maxDepth = 3;
    int candidateLimit = 10;
    int timeBudgetMs = 1000;
};

struct SearchResult
{
    int row = -1;
    int col = -1;
    bool pass = true;
    bool cancelled = false;
    int completedDepth = 0;
    int nodes = 0;
    qint64 elapsedMs = 0;
};

class MediumAI
{
public:
    explicit MediumAI(SearchOptions options = {});
    const SearchOptions &options() const { return m_options; }
    SearchResult chooseMove(const GoMatch &position,
                            const std::shared_ptr<std::atomic_bool> &cancel = {}) const;

private:
    class Search;
    SearchOptions m_options;
    double evaluate(const GoMatch &game) const;
};
