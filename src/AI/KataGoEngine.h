#pragma once
#include "HardAI.h"

class KataGoEngine final : public HardAI::Engine
{
public:
    struct Options
    {
        QString executablePath;
        QString modelPath;
        QString configPath;
        int timeBudgetMs = 1800;
        int searchThreads = 4;
        bool fp16 = true;
    };
    explicit KataGoEngine(Options options);
    ~KataGoEngine() override;
    void prepare();
    SearchResult chooseMove(const GoMatch &position,
                            const std::shared_ptr<std::atomic_bool> &cancel = {}) override;
    QString name() const override;

private:
    struct Impl;
    Options m_options;
    std::unique_ptr<Impl> m_impl;
};
