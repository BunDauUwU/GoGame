#pragma once
#include "HardAI.h"
#include <mutex>
class KataGoEvaluator final : public HardAI::Evaluator {
public:
    struct Options {
        QString modelPath;
        int batchSize = 4, cachePower = 14, gpu = 0, cpuThreads = 2;
        bool fp16 = true;
        double duty = 0.30; // Approximate pacing, not a hard GPU utilization cap.
    };
    struct Statistics { std::uint64_t rows = 0, batches = 0, hits = 0; };
    explicit KataGoEvaluator(Options options);
    ~KataGoEvaluator() override;
    void prepare(); // Load and warm up once before attaching to a running game.
    HardAI::Evaluation evaluate(const HardAI::Position&, const HardAI::LegalMask&, const std::atomic_bool*) override;
    QString name() const override;
    Statistics statistics() const;
    static bool available();
private:
    struct Impl;
    Options m_options;
    std::unique_ptr<Impl> m_impl;
    std::once_flag m_prepared;
};
