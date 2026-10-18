#pragma once
#include "../Rules/GoMatch.h"
#include <atomic>
#include <memory>

struct SearchOptions
{
    int maxDepth = 5;
    int candidateLimit = 20;
    int timeBudgetMs = 2000;
    int stableIterations = 3;
    int minimumStopDepth = 3;
    double improvementEpsilon = 0.05;
    double passMargin = 0.0;
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
    double estimatedScore = 0;
    double passScore = 0;
    double moveScore = 0;
    double gainOverPass = 0;
    bool stoppedEarly = false;
    QString stopReason;
};

class HardAI
{
public:
    explicit HardAI(SearchOptions options = {});
    const SearchOptions &options() const { return m_options; }
    SearchResult chooseMove(const GoMatch &cur,
                            const std::shared_ptr<std::atomic_bool> &cancel = {}) const;

private:
    class Search;
    SearchOptions m_options;
    double evaluate(const GoMatch &game) const;
};

