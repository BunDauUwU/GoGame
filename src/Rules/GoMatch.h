#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <array>
#include <bitset>
#include <memory>
#include <vector>

class GoMatch {
public:
  static constexpr int Size = 19;
  static constexpr int Count = Size * Size;
  enum class ScoringPhase { Current, Final };
  struct Groups {
    std::array<int, Count> parent;
    std::array<int, Count> size{};
    std::array<std::bitset<Count>, Count> liberties;
    int root(int pos) const {
      while (parent[pos] != pos)
        pos = parent[pos];
      return pos;
    }
  };
  struct RecordedMove { int player; int point; }; // pass = Count
  struct Replay {
    std::array<int, Count> initialCells{};
    int initialPlayer = 0, initialKo = -1, initialPasses = 0, initialMoveCount = 0;
    std::array<int, 2> initialCaptured{};
    std::vector<RecordedMove> moves;
  };
  Replay replay() const;
  GoMatch();
  bool play(int row, int col, QString &error);
  bool validMove(int row, int col) const;
  void pass();
  double score() const;
  QJsonObject scoreDetails(ScoringPhase phase = ScoringPhase::Current) const;
  QJsonArray state() const;
  QJsonObject toJson() const;
  static bool fromJson(const QJsonObject &json, GoMatch &game, QString &error);
  const std::array<int, Count> &cells() const { return board; }
  const Groups &groups() const;
  int currentPlayer = 0;
  int moveCount = 0;
  int consecutivePasses = 0;
  int capturedLastMove = 0;
  int koPoint = -1;
  bool koThreatLastMove = false;
  std::array<int, 2> capturedStones{};
  static const std::array<std::array<int, 4>, Count> &neighbors();

private:
  std::shared_ptr<Replay> m_replay;
  void recordMove(int player, int point);
  std::array<int, Count> board;
  std::array<int, Count> previousBoard;
  bool hasPrevious = false;
  // Copies share an immutable DSU cache; each successful move builds its own
  // cache.
  mutable std::shared_ptr<const Groups> m_groups;
  static std::shared_ptr<const Groups>
  buildGroups(const std::array<int, Count> &board);
};
