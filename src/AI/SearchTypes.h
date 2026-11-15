#pragma once
#include <QString>
#include <cstddef>
#include <cstdint>

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
    double winValue = 0;
    QString error;
    std::uint64_t evaluations = 0, cacheHits = 0;
    std::uint32_t rootVisits = 0, reusedVisits = 0;
    std::size_t allocatedBytes = 0;
    bool memoryLimited = false;

    bool policy = false;
    bool heuristic = false;
};

