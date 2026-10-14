#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <array>
#include <bitset>
#include <memory>

class GoMatch {
public:
  static constexpr int Size = 19;
  static constexpr int Count = Size * Size;
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
  GoMatch();
  bool play(int row, int col, QString &error);
  bool validMove(int row, int col) const;
  void pass();
  double score() const;
  QJsonObject scoreDetails() const;
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
  std::array<int, Count> board;
  std::array<int, Count> previousBoard;
  bool hasPrevious = false;
  // Copies share an immutable DSU cache; each successful move builds its own
  // cache.
  mutable std::shared_ptr<const Groups> m_groups;
  static std::shared_ptr<const Groups>
  buildGroups(const std::array<int, Count> &board);
};
