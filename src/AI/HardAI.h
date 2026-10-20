#pragma once
#include "SearchTypes.h"
#include "../Rules/GoMatch.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

struct HardSearchOptions {
    int timeBudgetMs = 1800;
    int memoryMB = 64; // Arena/cache only; model and history journal are additional.
    int maxDepth = 256;
    std::uint32_t maxSimulations = 100000;
    int searchThreads = 1; // Concurrent leaves are batched by the native backend.
    float cpuct = 1.5f;
    float fpuReduction = 0.2f;
};

class HardAI {
public:
    static constexpr int PolicySize = GoMatch::Count + 1;
    using LegalMask = std::array<std::uint8_t, PolicySize>;
    struct History {
        std::shared_ptr<const History> parent;
        std::shared_ptr<const GoMatch::Replay> setup;
        int player = -1, point = -1;
        std::uint64_t hash = 0;
    };
    // Snapshot contract for a replaceable policy/value backend.
    struct Position {
        std::array<int, GoMatch::Count> cells{}; // -1 empty, 0 black, 1 white
        int currentPlayer = 0;
        int koPoint = -1;
        int consecutivePasses = 0;
        int moveCount = 0;
        std::array<int, 2> capturedStones{};
        double komi = 6.5;
        std::shared_ptr<const History> history;
    };
    struct Evaluation {
        std::array<float, PolicySize> policyLogits{}; // row*19+col; pass=361
        float value = 0; // [-1,1], from position.currentPlayer's perspective
    };
    // Implementations must be thread-safe when searchThreads > 1.
    class Evaluator {
    public:
        virtual ~Evaluator() = default;
        virtual Evaluation evaluate(const Position& position, const LegalMask& legal,
                                    const std::atomic_bool* cancel) = 0;
        virtual QString name() const { return QStringLiteral("Custom policy/value"); }
    };

    explicit HardAI(HardSearchOptions options = {});
    const HardSearchOptions& options() const { return m_options; }
    SearchResult chooseMove(const GoMatch& position,
                            const std::shared_ptr<std::atomic_bool>& cancel = {}) const;
    // Install/load the model once in this backend. Copies already executing keep
    // their old backend; changes don't block the GUI waiting for that search.
    void setEvaluator(std::shared_ptr<Evaluator> evaluator);
    void reset();
    QString backendName() const;
    static std::array<float, 5 * GoMatch::Count> inputPlanes(const Position&, const LegalMask&);
private:
    struct Impl;
    HardSearchOptions m_options;
    std::shared_ptr<Impl> m_impl;
};
