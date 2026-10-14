#include "Game.h"

#include "../Network/Protocol.h"

#include <vector>
#include <set>
#include <QJsonArray>

struct piece {
    int par;
    std::set<int> liberties;
    std::vector<int> group;
    int liberty() { return liberties.size(); }
} pieces[19*19+1];

int pos_to_int(int x, int y) {
    return x * 19 + y;
}

std::pair<int,int> int_to_pos (int x) {
    return std::make_pair(x / 19, x % 19);
}

int find_par(int x) {
    return x == pieces[x].par ? x : pieces[x].par = find_par(pieces[x].par);
}

bool Union(int u, int v) {
    int a = find_par(u);
    int b = find_par(v);
    if (a == b) return 0;

    if (pieces[a].liberties.size() < pieces[b].liberties.size()) std::swap(a, b);
    pieces[b].par = a;
    pieces[a].liberties.insert(pieces[b].liberties.begin(), pieces[b].liberties.end());
    pieces[b].liberties.clear();
    pieces[a].group.insert(pieces[a].group.end(), pieces[b].group.begin(), pieces[b].group.end());
    pieces[b].group.clear();
    return 1;
}


GameManager::GameManager(QObject* parent) : QObject(parent)
{
    m_board_state.resize(19, std::vector<int>(19, -1));

    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 19; j++) {
            int current_idx = pos_to_int(i, j);
            pieces[current_idx].par = current_idx;
            pieces[current_idx].group = {current_idx};
            pieces[current_idx].liberties.clear();

            for (int d = 0; d < 4; d++) {
                int ni = i + dx[d];
                int nj = j + dy[d];
                if (ni >= 0 && ni < 19 && nj >= 0 && nj < 19) {
                    pieces[current_idx].liberties.insert(pos_to_int(ni, nj));
                }
            }
        }
    }
}

void GameManager::setNetworkClient(NetworkClient* networkClient)
{
    if (m_networkClient) disconnect(m_networkClient, nullptr, this, nullptr);
    m_networkClient = networkClient;
    if (networkClient) {
        connect(networkClient, &NetworkClient::disconnected, this, [this] {
            if (!online() || m_finished) return;
            m_pending = false;
            m_finished = true;
            m_resultMessage = QStringLiteral("Disconnected from server; match cannot continue");
            emit interactionChanged();
            emit gameError(m_resultMessage);
        });
    }
}

QString GameManager::matchId() const
{
    return m_matchId;
}

int GameManager::playerIndex() const
{
    return m_playerIndex;
}

bool GameManager::is_valid_move(int x, int y)
{
    if (x < 0 || x >= 19 || y < 0 || y >= 19) return false;
    if (m_board_state[x][y] != -1) return false;

    if (online()) {
        if (!canAct()) return false;
        auto candidate = m_board_state;
        candidate[x][y] = m_currentPlayer;
        auto neighbors = [](int pos) {
            std::vector<int> cells;
            if (pos / 19 > 0) cells.push_back(pos - 19);
            if (pos / 19 < 18) cells.push_back(pos + 19);
            if (pos % 19 > 0) cells.push_back(pos - 1);
            if (pos % 19 < 18) cells.push_back(pos + 1);
            return cells;
        };
        auto group = [&](int pos, bool &liberty) {
            std::vector<int> cells{pos}; std::set<int> visited{pos}; liberty = false;
            const int color = candidate[pos / 19][pos % 19];
            for (size_t i = 0; i < cells.size(); ++i) for (int next : neighbors(cells[i])) {
                int state = candidate[next / 19][next % 19];
                if (state == -1) liberty = true;
                else if (state == color && visited.insert(next).second) cells.push_back(next);
            }
            return cells;
        };
        int pos = x * 19 + y;
        for (int next : neighbors(pos)) if (candidate[next / 19][next % 19] == 1 - m_currentPlayer) {
            bool liberty; auto cells = group(next, liberty);
            if (!liberty) for (int cell : cells) candidate[cell / 19][cell % 19] = -1;
        }
        bool liberty; group(pos, liberty);
        return liberty && candidate != m_previousOnlineBoard;
    }

    int current_idx = pos_to_int(x, y);
    if (current_idx == m_ko_square) return false;

    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    bool empty_square = false;
    bool can_connect = false;
    int captured = 0;

    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if (nx >= 0 && nx < 19 && ny >= 0 && ny < 19) {
            int neighbor_color = m_board_state[nx][ny];

            if (neighbor_color == -1) {
                empty_square = true;
            }
            else if (neighbor_color == m_currentPlayer) {
                int p = find_par(pos_to_int(nx, ny));
                if (pieces[p].liberties.size() > 1) {
                    can_connect = true;
                }
            }
            else {
                int p = find_par(pos_to_int(nx, ny));
                if (pieces[p].liberties.size() == 1 && pieces[p].liberties.count(current_idx)) {
                    captured += pieces[p].group.size();
                }
            }
        }
    }

    if (!empty_square && !can_connect && captured == 0) {
        return false;
    }

    return true;
}


void GameManager::update_pieces(int x, int y)
{
    if (!is_valid_move(x, y)) { emit gameError(QStringLiteral("Invalid move or not your turn")); return; }
    if (online()) { submit(GameCommandType::Play, {{"row", x}, {"col", y}}); return; }

    int current_stone = pos_to_int(x, y);
    m_board_state[x][y] = m_currentPlayer;

    // reset
    pieces[current_stone].par = current_stone;
    pieces[current_stone].group = {current_stone};
    pieces[current_stone].liberties.clear();

    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx >= 0 && nx < 19 && ny >= 0 && ny < 19) {
            if (m_board_state[nx][ny] == -1) {
                pieces[current_stone].liberties.insert(pos_to_int(nx, ny));
            }
        }
    }

    // Ko solution
    // Chỉ cần lưu 1 ô vì theo lí thuyết không thể đặt ô kín khí lớn => chỉ còn luật Ko
    int total_captured_stones = 0;
    int last_captured_stone_idx = -1;
    std::set<int> groups_to_capture;


    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx >= 0 && nx < 19 && ny >= 0 && ny < 19) {
            if (m_board_state[nx][ny] != -1) {
                int neighbor_par = find_par(pos_to_int(nx, ny));
                pieces[neighbor_par].liberties.erase(current_stone);

                if (m_board_state[nx][ny] != m_currentPlayer && pieces[neighbor_par].liberty() == 0) {
                    groups_to_capture.insert(neighbor_par);
                }
            }
        }
    }

    for (int target_par : groups_to_capture) {
        total_captured_stones += pieces[target_par].group.size();

        const auto captured_group = pieces[target_par].group;
        for (int stone_idx : captured_group) {
            last_captured_stone_idx = stone_idx;
            std::pair<int, int> s_pos = int_to_pos(stone_idx);

            m_board_state[s_pos.first][s_pos.second] = -1;

            for (int d = 0; d < 4; d++) {
                int nnx = s_pos.first + dx[d];
                int nny = s_pos.second + dy[d];
                if (nnx >= 0 && nnx < 19 && nny >= 0 && nny < 19) {
                    if (m_board_state[nnx][nny] != -1) {
                        int p_neighbor = find_par(pos_to_int(nnx, nny));
                        pieces[p_neighbor].liberties.insert(stone_idx);
                    }
                }
            }

            pieces[stone_idx].par = stone_idx;
            pieces[stone_idx].group = {stone_idx};
            pieces[stone_idx].liberties.clear();
            for (int d = 0; d < 4; d++) {
                int nnx = s_pos.first + dx[d];
                int nny = s_pos.second + dy[d];
                if (nnx >= 0 && nnx < 19 && nny >= 0 && nny < 19) {
                    if (m_board_state[nnx][nny] == -1) {
                        pieces[stone_idx].liberties.insert(pos_to_int(nnx, nny));
                    }
                }
            }
        }
    }

    // Total Point
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx >= 0 && nx < 19 && ny >= 0 && ny < 19) {
            if (m_board_state[nx][ny] == -1) {
                pieces[current_stone].liberties.insert(pos_to_int(nx, ny));
            }
        }
    }

    // Union
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx >= 0 && nx < 19 && ny >= 0 && ny < 19) {
            if (m_board_state[nx][ny] == m_currentPlayer) {
                Union(current_stone, pos_to_int(nx, ny));
            }
        }
    }

    // Ko solve
    int final_par = find_par(current_stone);
    if (total_captured_stones == 1 && pieces[final_par].group.size() == 1 && pieces[final_par].liberty() == 1) {
        m_ko_square = last_captured_stone_idx;
    } else {
        m_ko_square = -1;
    }

    m_currentPlayer = 1 - m_currentPlayer;
    emit currentPlayerChanged();
    emit boardStateChanged();
}

void GameManager::pass_turn()
{
    if (online()) { submit(GameCommandType::EndRound); return; }
    // A pass is a turn, so an immediate-ko restriction no longer applies.
    m_ko_square = -1;
    m_currentPlayer = 1 - m_currentPlayer;
    emit currentPlayerChanged();
}


void GameManager::setMatchInfo(const QString& matchId, int playerIndex, const QString& opponentName) {
    if (playerIndex != 0 && playerIndex != 1)
        return;
    if (m_matchId == matchId && m_playerIndex == playerIndex && m_opponentName == opponentName)
        return;
    m_board_state.assign(19, std::vector<int>(19, -1));
    m_previousOnlineBoard.clear();
    m_snapshot.clear();
    m_currentPlayer = 0;
    m_ready = false; m_finished = false; m_pending = false;
    m_resultMessage.clear();
    m_matchId = matchId;
    m_playerIndex = playerIndex;
    m_opponentName = opponentName;
    emit matchInfoChanged();
    emit boardStateChanged(); emit currentPlayerChanged(); emit snapshotChanged();
    emit interactionChanged();
}

void GameManager::handleServerMessage(const QJsonObject& message) {
    const QString type = Protocol::readMessageType(message);
    const QJsonObject payload = Protocol::readPayload(message);

    if (type == "MatchFound") {
        setMatchInfo(payload.value("matchId").toString(), payload.value("playerIndex").toInt(-1), payload.value("opponentName").toString());
        return;
    }

    if (type == "GameSnapshot") {
        const QString matchId = payload.value("matchId").toString();

        if (matchId.isEmpty() || matchId != m_matchId) {
            return;
        }

        const auto board = payload.value("board").toArray();
        const int current = payload.value("currentPlayerIndex").toInt(-1);
        if (board.size() != 361 || (current != 0 && current != 1)) return;
        for (const auto &cell : board) {
            if (!cell.isDouble() || (cell.toDouble() != -1 && cell.toDouble() != 0 && cell.toDouble() != 1)) return;
        }
        const int oldMoves = m_snapshot.value("moveCount", -1).toInt();
        const int newMoves = payload.value("moveCount").toInt(-1);
        if (oldMoves >= 0 && newMoves < oldMoves) return;
        if (m_ready && newMoves == oldMoves + 1) m_previousOnlineBoard = m_board_state;
        else if (newMoves != oldMoves) m_previousOnlineBoard.clear();
        const auto oldBoard = m_board_state;
        const int oldCurrent = m_currentPlayer;
        const bool oldCanAct = canAct();
        const bool oldFinished = m_finished;
        for (int i = 0; i < 361; ++i) m_board_state[i / 19][i % 19] = board[i].toInt();
        m_currentPlayer = current;
        m_ready = true;
        if (newMoves != oldMoves || !payload.value("started").toBool()) m_pending = false;
        m_finished = !payload.value("started").toBool();
        if (m_finished) m_resultMessage = payload.value("endReason").toString();
        if (oldBoard != m_board_state) emit boardStateChanged();
        if (oldCurrent != m_currentPlayer) emit currentPlayerChanged();
        if (oldCanAct != canAct() || oldFinished != m_finished) emit interactionChanged();
        m_snapshot = payload.toVariantMap();
        emit snapshotChanged();

        return;
    }

    if (type == "GameEvent") {
        const QString matchId = payload.value("matchId").toString();

        if (matchId.isEmpty() || matchId != m_matchId) {
            return;
        }

        const QJsonArray events = payload.value("events").toArray();

        for (const QJsonValue &value : events) {
            const QJsonObject event = value.toObject();
            const QString eventType = event.value("eventType").toString();

            if (eventType == "GameEnded") {
                m_finished = true; m_pending = false;
                m_resultMessage = event.value("reason").toString();
                emit interactionChanged();
            }
        }

        return;
    }

    if (type == "Error" && online()) {
        m_pending = false;
        emit interactionChanged();
        emit gameError(payload.value("message").toString());
    }
}


void GameManager::sendCommand(const QJsonObject &command) {
    if (!m_networkClient) {
        emit gameError(QStringLiteral("Network client is not ready"));
        return;
    }

    if (!m_networkClient->isConnected()) {
        emit gameError(QStringLiteral("Not connected to server"));
        return;
    }

    m_networkClient->sendMessage(command);
}

void GameManager::submit(GameCommandType type, const QJsonObject &command) {
    if (!online() || !m_ready || m_finished || m_pending || !m_networkClient || !m_networkClient->isConnected()
        || (type != GameCommandType::Concede && !canAct())) {
        emit gameError(QStringLiteral("Cannot send command: wait for your turn or server response"));
        return;
    }
    m_pending = true;
    emit interactionChanged();
    sendCommand(Protocol::makeGameCommand(m_matchId, type, command));
}
void GameManager::concede() { submit(GameCommandType::Concede); }