import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components/battle"
import "../Components/controls"
import "../Core"
import "../Singletons"
import GoGame

Rectangle {
    id: boardScreen
    // 0 = black, 1 = white. Local play defaults to the black perspective.
    property var battleState: gameManager.snapshot || ({})
    property int localPlayerColor: gameManager.playerIndex === 1 ? 1 : 0
    readonly property bool boardFlipped: localPlayerColor === 1
    property string localPlayerName: "Nguoi Dep Trai"
    property string opponentPlayerName: "Nguoi Bi Gay Chim"
    property url localAvatar: "qrc:/GoGame/assets/playerAvatar/ayame2.png"
    property url opponentAvatar: "qrc:/GoGame/assets/playerAvatar/ayame1.png"
    property int cur_x: -1
    property int cur_y: -1
    property string errorMessage: ""
    property bool previewEnabled: true

    function clearPreview() {
        cur_x = -1;
        cur_y = -1;
    }

    function selectIntersection(row, col) {
        if (!gameManager.is_valid_move(row, col)) {
            clearPreview();
            errorMessage = qsTr("Invalid move");
            return;
        }
        errorMessage = "";
        if (!previewEnabled || (cur_x === row && cur_y === col)) {
            clearPreview();
            gameManager.update_pieces(row, col);
        } else {
            cur_x = row;
            cur_y = col;
        }
    }

    onPreviewEnabledChanged: clearPreview()
    onLocalPlayerColorChanged: clearPreview()

    property double snapshotReceivedAt: Date.now()
    property double clockNow: Date.now()
    Timer {
        interval: 200
        repeat: true
        running: gameManager.online && !gameManager.finished
        onTriggered: boardScreen.clockNow = Date.now()
    }

    Connections {
        target: gameManager
        function onGameError(message) {
            boardScreen.clearPreview();
            boardScreen.errorMessage = message;
        }
        function onSnapshotChanged() {
            boardScreen.snapshotReceivedAt = Date.now();
            boardScreen.clockNow = Date.now();
        }
        function onInteractionChanged() {
            boardScreen.clearPreview();
        }
        function onCurrentPlayerChanged() {
            boardScreen.clearPreview();
        }
        function onBoardStateChanged() {
            boardScreen.clearPreview();
        }
    }

    implicitWidth: 1280
    implicitHeight: 720

    Image {
        id: background

        anchors.fill: parent
        property int backgroundIndex: 1

        source: AssetManager.url("Background" + AppState.backgroundIndex)
        fillMode: Image.PreserveAspectCrop
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 12

        Text {
            text: gameManager.finished ? qsTr("Game ended") : gameManager.pending ? qsTr("Waiting for opponent…") : gameManager.online ? (gameManager.canAct ? qsTr("Your turn") : qsTr("Opponent's turn")) : gameManager.currentPlayer === 0 ? qsTr("Black to play") : qsTr("White to play")
            font.pixelSize: 20
        }

        Text {
            id: error
            text: errorMessage
            color: "red"
        }
    }

    Rectangle {
        id: board
        objectName: "goBoard"
        rotation: boardScreen.boardFlipped ? 180 : 0
        transformOrigin: Item.Center
        anchors.centerIn: parent
        width: Math.max(0, Math.min(608, parent.width - 32, parent.height - 80))
        height: width
        color: "#80deb887"
        property int boardSize: 19
        property real margin: 20
        property var boardState: gameManager.boardState

        Canvas {
            id: gridCanvas
            anchors.fill: parent
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);

                var usableWidth = width - (2 * board.margin);
                var usableHeight = height - (2 * board.margin);
                var stepX = usableWidth / (board.boardSize - 1);
                var stepY = usableHeight / (board.boardSize - 1);

                ctx.strokeStyle = "#222222";
                ctx.lineWidth = 1;

                ctx.beginPath();
                for (let i = 0; i < board.boardSize; i++) {
                    let y = board.margin + i * stepY;
                    ctx.moveTo(board.margin, y);
                    ctx.lineTo(width - board.margin, y);

                    let x = board.margin + i * stepX;
                    ctx.moveTo(x, board.margin);
                    ctx.lineTo(x, height - board.margin);
                }
                ctx.stroke();

                drawHoshi(ctx, stepX, stepY);
            }

            function drawHoshi(ctx, stepX, stepY) {
                ctx.fillStyle = "#222222";
                var hoshiPoints = [3, 9, 15];

                if (board.boardSize === 19) {
                    hoshiPoints = [3, 9, 15];
                } else if (board.boardSize === 13) {
                    hoshiPoints = [3, 6, 9];
                } else if (board.boardSize === 9) {
                    hoshiPoints = [2, 4, 6];
                }

                for (let r = 0; r < hoshiPoints.length; r++) {
                    for (let c = 0; c < hoshiPoints.length; c++) {
                        let gx = hoshiPoints[c];
                        let gy = hoshiPoints[r];
                        let px = board.margin + gx * stepX;
                        let py = board.margin + gy * stepY;

                        ctx.beginPath();
                        ctx.arc(px, py, 4, 0, Math.PI * 2);
                        ctx.fill();
                    }
                }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            enabled: gameManager.canAct
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: function (mouse) {
                var usableWidth = width - (2 * board.margin);
                var usableHeight = height - (2 * board.margin);
                var stepX = usableWidth / (board.boardSize - 1);
                var stepY = usableHeight / (board.boardSize - 1);

                var col = Math.round((mouse.x - board.margin) / stepX);
                var row = Math.round((mouse.y - board.margin) / stepY);

                if (col >= 0 && col < board.boardSize && row >= 0 && row < board.boardSize) {
                    boardScreen.selectIntersection(row, col);
                }
            }
        }

        Rectangle {
            id: previewStone
            objectName: "previewStone"
            z: 1
            visible: boardScreen.previewEnabled && boardScreen.cur_x >= 0 && boardScreen.cur_y >= 0
            width: (board.width - 2 * board.margin) / (board.boardSize - 1) * 0.9
            height: width
            radius: width / 2
            x: board.margin + boardScreen.cur_y * (board.width - 2 * board.margin) / (board.boardSize - 1) - width / 2
            y: board.margin + boardScreen.cur_x * (board.height - 2 * board.margin) / (board.boardSize - 1) - height / 2
            color: gameManager.currentPlayer === 0 ? "#111111" : "#f0f0f0"
            border.color: gameManager.currentPlayer === 0 ? "#000000" : "#cccccc"
            border.width: 1
            scale: 0.95
            opacity: 0.55

            SequentialAnimation on opacity {
                running: previewStone.visible
                loops: Animation.Infinite
                NumberAnimation {
                    from: 0.8
                    to: 0.3
                    duration: 350
                    easing.type: Easing.InOutSine
                }
                NumberAnimation {
                    from: 0.3
                    to: 0.8
                    duration: 350
                    easing.type: Easing.InOutSine
                }
            }
        }

        Repeater {
            model: board.boardSize * board.boardSize

            delegate: Item {
                id: stoneDelegate
                property int cellIndex: index

                property int stateValue: board.boardState && board.boardState[cellIndex] !== undefined ? board.boardState[cellIndex] : -1

                visible: stateValue !== -1

                width: {
                    let usableWidth = board.width - (2 * board.margin);
                    return (usableWidth / (board.boardSize - 1)) * 0.9;
                }
                height: width

                x: {
                    let usableWidth = board.width - (2 * board.margin);
                    let stepX = usableWidth / (board.boardSize - 1);
                    let col = cellIndex % board.boardSize;
                    return board.margin + col * stepX - width / 2;
                }

                y: {
                    let usableHeight = board.height - (2 * board.margin);
                    let stepY = usableHeight / (board.boardSize - 1);
                    let row = Math.floor(cellIndex / board.boardSize);
                    return board.margin + row * stepY - height / 2;
                }

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2

                    color: stoneDelegate.stateValue === 0 ? "#111111" : "#f0f0f0"
                    border.color: stoneDelegate.stateValue === 0 ? "#000000" : "#cccccc"
                    border.width: 1

                    scale: 0.95
                }
            }
        }
    }

    Button {
        anchors.right: parent.right
        anchors.bottom: userInfo.top
        anchors.bottomMargin: 112
        anchors.rightMargin: 20
        text: qsTr("Pass")
        enabled: gameManager.canAct
        onClicked: gameManager.pass_turn()
    }

    Button {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
        text: qsTr("Concede")
        visible: gameManager.online
        enabled: !gameManager.finished && !gameManager.pending
        onClicked: concedeDialog.open()
    }
    Dialog {
        id: concedeDialog
        anchors.centerIn: parent
        title: qsTr("Concede this game?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: gameManager.concede()
    }
    Rectangle {
        anchors.centerIn: parent
        width: 380
        height: resultColumn.implicitHeight + 40
        radius: 12
        color: "#eef0ede7"
        border.color: "#bda78c"
        z: 10
        visible: gameManager.online && gameManager.finished
        Column {
            id: resultColumn
            anchors.centerIn: parent
            width: parent.width - 40
            spacing: 14
            Text {
                width: parent.width
                text: boardScreen.battleState.winnerIndex === gameManager.playerIndex ? qsTr("You won") : boardScreen.battleState.winnerIndex >= 0 ? qsTr("You lost") : qsTr("Game ended")
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 24
            }
            Text {
                width: parent.width
                text: gameManager.resultMessage
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Back to menu")
                onClicked: stack.pop()
            }
        }
    }

    PlayerPanel {
        id: opponentInfo
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 20
        width: Math.min(420, parent.width * 0.25)
        height: width * 90 / 420
        playerName: boardScreen.opponentPlayerName
        avatarSource: boardScreen.opponentAvatar
    }

    PlayerPanel {
        id: userInfo
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        width: Math.min(420, parent.width * 0.25)
        height: width * 90 / 420
        playerName: boardScreen.localPlayerName
        avatarSource: boardScreen.localAvatar
        avatarOnRight: true
    }
    GameClock {
        objectName: "opponentClock"
        z: 5
        anchors.left: opponentInfo.left
        anchors.top: opponentInfo.bottom
        anchors.topMargin: 12
        width: opponentInfo.width
        playerLabel: boardScreen.opponentPlayerName
        property int colorIndex: 1 - boardScreen.localPlayerColor
        clockState: (boardScreen.battleState.clocks || [])[colorIndex] || ({})
        elapsedMs: boardScreen.clockNow - boardScreen.snapshotReceivedAt
        active: boardScreen.battleState.currentPlayerIndex === colorIndex
        finished: gameManager.finished
        visible: gameManager.online
    }
    GameClock {
        objectName: "localClock"
        z: 5
        anchors.right: userInfo.right
        anchors.bottom: userInfo.top
        anchors.bottomMargin: 12
        width: userInfo.width
        playerLabel: boardScreen.localPlayerName
        clockState: (boardScreen.battleState.clocks || [])[boardScreen.localPlayerColor] || ({})
        elapsedMs: boardScreen.clockNow - boardScreen.snapshotReceivedAt
        active: boardScreen.battleState.currentPlayerIndex === boardScreen.localPlayerColor
        finished: gameManager.finished
        visible: gameManager.online
    }
}
