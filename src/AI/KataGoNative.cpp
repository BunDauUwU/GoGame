#include "KataGoNative.h"
#include "core/config_parser.h"
#include "neuralnet/nneval.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
namespace GoKata {
namespace {
std::mutex dutyMutex;
std::unordered_map<const void *, double> duties;
} // namespace
double inferenceDuty(const void *evaluator) {
  std::lock_guard<std::mutex> lock(dutyMutex);
  const auto found = duties.find(evaluator);
  return found == duties.end() ? 1. : found->second;
}
struct Native::Impl {
  Logger logger{nullptr, false, true};
  ConfigParser config;
  std::unique_ptr<NNEvaluator> net;
  ~Impl() {
    const auto *evaluator = net.get();
    net.reset();
    std::lock_guard<std::mutex> lock(dutyMutex);
    duties.erase(evaluator);
  }
};
Native::Native(const std::string &model, int batchSize, int cachePower, int gpu,
               int cpuThreads, bool fp16, double duty)
    : impl(std::make_unique<Impl>()) {
  static std::once_flag init;
  std::call_once(init, [] {
    Board::initHash();
    NeuralNet::globalInitialize();
  });
#ifdef USE_EIGEN_BACKEND
  const int serverThreads = cpuThreads;
#else
  (void)cpuThreads;
  const int serverThreads = 1;
#endif
  impl->net = std::make_unique<NNEvaluator>(
      "HardAI", model, "", &impl->logger, batchSize, 19, 19, true, true,
      cachePower, 8, false, "", fp16 ? enabled_t::Auto : enabled_t::False,
      serverThreads, std::vector<int>(serverThreads, gpu), "GoGame-HardAI",
      false, 0, impl->config);
  if (impl->net->requiresSGFMetadata())
    throw std::runtime_error(
        "Use a normal KataGo playing model, not a HumanSL profile model");
  {
    std::lock_guard<std::mutex> lock(dutyMutex);
    duties[impl->net.get()] = duty;
  }
  impl->net->spawnServerThreads();
  Request warm;
  warm.initial.fill(-1);
  warm.current.fill(-1);
  evaluate(warm);
}
Native::~Native() = default;
Statistics Native::statistics() const {
  return {impl->net->numRowsProcessed(), impl->net->numBatchesProcessed(),
          impl->net->numCacheHits()};
}
Result Native::evaluate(const Request &request) {
  Board board(19, 19);
  for (int i = 0; i < 361; ++i) {
    if (request.initial[i] < -1 || request.initial[i] > 1)
      throw std::runtime_error("Invalid KataGo setup color");
    if (request.initial[i] >= 0 &&
        !board.setStone(Location::getLoc(i % 19, i / 19, 19),
                        request.initial[i] == 0 ? P_BLACK : P_WHITE))
      throw std::runtime_error("Invalid KataGo setup stones");
  }
  board.ko_loc = request.initialKo < 0
                     ? Board::NULL_LOC
                     : Location::getLoc(request.initialKo % 19,
                                        request.initialKo / 19, 19);
  board.numBlackCaptures = request.initialCaptured[1];
  board.numWhiteCaptures = request.initialCaptured[0];
  Rules rules(Rules::KO_SIMPLE, Rules::SCORING_TERRITORY, Rules::TAX_NONE,
              false, false, Rules::WHB_ZERO, false, float(request.komi));
  bool supported = false;
  impl->net->getSupportedRules(rules, supported);
  if (!supported)
    throw std::runtime_error("KataGo model does not support this game's rules");
  BoardHistoryModes modes(
      impl->net->modelPreferPassAliveUnderSuicideRules(),
      impl->net->modelPreferExcludeTerritoryAdjacentToAtari());
  Player next = request.initialPlayer == 0 ? P_BLACK : P_WHITE;
  BoardHistory history(board, next, rules, 0, modes);
  history.setInitialTurnNumber(request.initialTurn);
  history.consecutiveEndingPasses = request.initialPasses;
  for (const auto &move : request.moves) {
    if (move.player < 0 || move.player > 1 || move.point < 0 ||
        move.point > 361)
      throw std::runtime_error("Invalid KataGo history move");
    const Player pla = move.player == 0 ? P_BLACK : P_WHITE;
    const Loc loc = move.point == 361 ? Board::PASS_LOC
                                      : Location::getLoc(move.point % 19,
                                                         move.point / 19, 19);
    if (pla != next || !board.isLegal(loc, pla, false))
      throw std::runtime_error("KataGo history contains an illegal move");
    // GoGame ends after two passes in one phase; don't enter Japanese encore.
    history.makeBoardMoveAssumeLegal(board, loc, pla, nullptr, true);
    next = getOpp(pla);
  }
  if (next != (request.player == 0 ? P_BLACK : P_WHITE))
    throw std::runtime_error("KataGo history player mismatch");
  for (int i = 0; i < 361; ++i) {
    const int color = board.colors[Location::getLoc(i % 19, i / 19, 19)];
    const int expected = request.current[i] < 0    ? C_EMPTY
                         : request.current[i] == 0 ? P_BLACK
                                                   : P_WHITE;
    if (color != expected)
      throw std::runtime_error(
          "KataGo history does not reproduce the position");
  }
  const Loc ko = request.ko < 0
                     ? Board::NULL_LOC
                     : Location::getLoc(request.ko % 19, request.ko / 19, 19);
  if (board.ko_loc != ko)
    throw std::runtime_error("KataGo history ko mismatch");
  thread_local NNResultBuf buffer;
  MiscNNInputParams params;
  impl->net->evaluate(board, history, next, params, buffer, false, false);
  const auto &output = *buffer.result;
  Result result;
  for (int i = 0; i < 362; ++i)
    result.policy[i] = output.policyProbs[i];
  result.value = (output.whiteWinProb - output.whiteLossProb) *
                 (next == P_WHITE ? 1.f : -1.f);
  return result;
}
} // namespace GoKata
