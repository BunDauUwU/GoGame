#include "KataGoEvaluator.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#ifdef GOGAME_KATAGO
#include "KataGoNative.h"
#endif
struct KataGoEvaluator::Impl
{
#ifdef GOGAME_KATAGO
    std::unique_ptr<GoKata::Native> native;
#endif
};
KataGoEvaluator::KataGoEvaluator(Options options)
    : m_options(std::move(options))
    , m_impl(std::make_unique<Impl>())
{
    if (m_options.modelPath.isEmpty() || m_options.batchSize < 1 || m_options.batchSize > 32
        || m_options.cachePower < 8 || m_options.cachePower > 20 || m_options.cpuThreads < 1
        || m_options.cpuThreads > 16 || m_options.gpu < 0 || !std::isfinite(m_options.duty)
        || m_options.duty <= 0 || m_options.duty > 1)
        throw std::invalid_argument("Invalid KataGo backend options");
}
KataGoEvaluator::~KataGoEvaluator() = default;
bool KataGoEvaluator::available()
{
#ifdef GOGAME_KATAGO
    return true;
#else
    return false;
#endif
}
void KataGoEvaluator::prepare()
{
    std::call_once(m_prepared, [&] {
#ifdef GOGAME_KATAGO
        m_impl->native = std::make_unique<GoKata::Native>(m_options.modelPath.toStdString(),
                                                          m_options.batchSize,
                                                          m_options.cachePower,
                                                          m_options.gpu,
                                                          m_options.cpuThreads,
                                                          m_options.fp16,
                                                          m_options.duty);
#else
            throw std::runtime_error("Build with -DGOGAME_ENABLE_KATAGO=ON to load KataGo models");
#endif
    });
}
KataGoEvaluator::Statistics KataGoEvaluator::statistics() const
{
#ifdef GOGAME_KATAGO
    if (m_impl->native) {
        auto s = m_impl->native->statistics();
        return {s.rows, s.batches, s.hits};
    }
#endif
    return {};
}
QString KataGoEvaluator::name() const
{
    return QStringLiteral("KataGo neural model + GoGame MCTS");
}
HardAI::Evaluation KataGoEvaluator::evaluate(const HardAI::Position &p,
                                             const HardAI::LegalMask &legal,
                                             const std::atomic_bool *cancel)
{
    if (cancel && cancel->load())
        throw std::runtime_error("KataGo evaluation cancelled");
    prepare();
#ifdef GOGAME_KATAGO
    if (!p.history || !p.history->setup)
        throw std::runtime_error("KataGo requires a real history or explicit setup");
    const auto &setup = *p.history->setup;
    GoKata::Request request;
    request.initial = setup.initialCells;
    request.initialPlayer = setup.initialPlayer;
    request.initialKo = setup.initialKo;
    request.initialPasses = setup.initialPasses;
    request.initialCaptured = setup.initialCaptured;
    request.initialTurn = setup.initialMoveCount;
    request.current = p.cells;
    request.player = p.currentPlayer;
    request.ko = p.koPoint;
    request.komi = p.komi;
    for (auto node = p.history; node && node->parent; node = node->parent)
        request.moves.push_back({node->player, node->point});
    std::reverse(request.moves.begin(), request.moves.end());
    const auto result = m_impl->native->evaluate(request);
    if (cancel && cancel->load())
        throw std::runtime_error("KataGo evaluation cancelled");
    HardAI::Evaluation e;
    e.value = result.value;
    for (int i = 0; i < HardAI::PolicySize; ++i)
        e.policyLogits[i] = legal[i] && std::isfinite(result.policy[i]) && result.policy[i] > 0
                                ? std::log(result.policy[i])
                                : -INFINITY;
    return e;
#else
    (void) p;
    (void) legal;
    return {};
#endif
}
