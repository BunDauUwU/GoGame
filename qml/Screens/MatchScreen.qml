import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import "../Components/battle"
import "../Components/controls"
import "../Core"
import "../Singletons"
import GoGame

Rectangle {
    id: boardScreen
    // 0 = black, 1 = white. Local play defaults to the black perspective.
    property var battleState: gameManager.snapshot || ({})
    readonly property var scoring: gameManager.scoring
    readonly property var serverScoring: gameManager.serverScoring
    readonly property var scoringComparison: gameManager.scoringComparison

    function pointsForColor(color) {
        var points = color === 0 ? scoring.blackPoints : scoring.whitePoints;
        return points === undefined ? (color === 0 ? 0 : 6.5) : points;
    }
    property int localPlayerColor: gameManager.playerIndex === 1 ? 1 : 0
    readonly property bool boardFlipped: !gameManager.sandbox && localPlayerColor === 1
    property string localPlayerName: "Nguoi Dep Trai"
    property string opponentPlayerName: "Nguoi Bi Gay Chim"
    property url localAvatar: "qrc:/GoGame/assets/playerAvatar/ayame2.png"
    property url opponentAvatar: "qrc:/GoGame/assets/playerAvatar/ayame1.png"
    property int cur_x: -1
    property int cur_y: -1
    property bool previewEnabled: true

    Component.onDestruction: {
        if (gameManager.aiEnabled)
            gameManager.stopAI();
        AppState.playingAgainstAI = false;
    }

    function clearPreview() {
        cur_x = -1;
        cur_y = -1;
    }

    function selectIntersection(row, col) {
        if (!gameManager.is_valid_move(row, col)) {
            clearPreview();
            return;
        }
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

    SoundEffect {
        id: placeSound
        source: AssetManager.url("PlaceSound")
        volume: AssetManager.sfxVolume
    }

    SoundEffect {
        id: errorSound
        source: AssetManager.url("ErrorSound")
        volume: AssetManager.sfxVolume
    }

    MediaPlayer {
        id: backgroundMusic

        source: AssetManager.url("BackgroundSong" + AssetManager.currentBackgroundSong)
        loops: MediaPlayer.Infinite

        audioOutput: AudioOutput {
            volume: AssetManager.bgmVolume
        }

        onSourceChanged: {
            stop();
            play();
        }

        Component.onCompleted: play()
    }

    Connections {
        target: gameManager
        function onGameError() {
            boardScreen.clearPreview();
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
        function onLastMoveCoordsChanged() {
            placeSound.play();
        }

        function onInvalidMovePlayed() {
            if (!errorSound.playing) {
                errorSound.play();
            }
        }
    }

    implicitWidth: 1280
    implicitHeight: 720

    Image {
        id: background

        anchors.fill: parent

        source: AssetManager.url("Background" + AssetManager.currentBackground)
        fillMode: Image.PreserveAspectCrop
    }

    GameButton {
        anchors.right: parent.right
        anchors.top: parent.top
        height: 40
        width: 200
        text: qsTr("Settings")
        onClicked: {
            stack.push("./SettingsScreen.qml");
        }
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 12

        Text {
            text: gameManager.finished ? qsTr("Game ended") : gameManager.pending ? (qsTr("Waiting for opponent…")) : gameManager.online ? (gameManager.canAct ? qsTr("Your turn") : qsTr("Opponent's turn")) : gameManager.currentPlayer === 0 ? qsTr("Black turn") : qsTr("White turn")
            font.pixelSize: 20
        }
        Text {
            objectName: "scoringComparisonStatus"
            visible: gameManager.online
            text: boardScreen.scoringComparison.status === "matched" ? qsTr("Client/server scores match") : boardScreen.scoringComparison.status === "mismatch" ? qsTr("Client/server scores differ") : qsTr("Server score unavailable")
            color: boardScreen.scoringComparison.status === "mismatch" ? "#b55f27" : "#394451"
            font.pixelSize: 13
            Layout.alignment: Qt.AlignHCenter
        }
    }

    Rectangle {
        z: 20
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 40, 580)
        height: noticeText.implicitHeight + 20
        radius: 8
        color: "#ed252c35"
        border.color: "#bda78c"
        visible: noticeTimer.running
        Text {
            id: noticeText
            anchors.centerIn: parent
            width: parent.width - 24
            text: gameManager.notification
            wrapMode: Text.WordWrap
            color: "#f3ede1"
            horizontalAlignment: Text.AlignHCenter
        }
        Timer {
            id: noticeTimer
            interval: 1000
        }
        Connections {
            target: gameManager
            function onNotificationChanged() {
                if (gameManager.notification.length > 0)
                    noticeTimer.restart();
                else
                    noticeTimer.stop();
            }
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

        // Clip the oversized, rotated texture to the board's edges.
        clip: true

        Item {
            id: rotatedTexture

            anchors.centerIn: parent

            width: board.width * 2
            height: board.height * 2

            rotation: 10

            Image {
                id: boardTexture
                anchors.fill: parent

                source: AssetManager.url("BoardTexture" + AssetManager.currentBoardTexture)

                fillMode: Image.Tile
                sourceSize: Qt.size(512, 512)

                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignVCenter

                smooth: true
                mipmap: true
                asynchronous: true
            }
        }

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
                readonly property var lastMove: gameManager.lastMoveCoords
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

                Image {
                    anchors.fill: parent

                    source: AssetManager.url((stoneDelegate.stateValue === 0 ? "Black" : "White") + AssetManager.currentStoneSkin)

                    fillMode: Image.PreserveAspectFit
                    sourceSize: Qt.size(128, 128)
                    asynchronous: true
                    smooth: true
                    mipmap: true
                }
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1.5

                    radius: width / 2
                    color: "transparent"

                    border.color: stoneDelegate.stateValue === 0 ? "#ffd54f"  // Gold for black stones
                    : "#d32f2f"  // Red for white stones
                    border.width: 2
                    antialiasing: true

                    visible: stoneDelegate.lastMove && stoneDelegate.lastMove.length >= 2 && Number(stoneDelegate.lastMove[0]) === Math.floor(stoneDelegate.cellIndex / board.boardSize) && Number(stoneDelegate.lastMove[1]) === stoneDelegate.cellIndex % board.boardSize
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 20
        width: Math.min(260, parent.width * 0.22)
        height: localActions.implicitHeight + 24
        radius: 12
        color: "#ed252c35"
        border.color: "#bda78c"
        visible: gameManager.sandbox || gameManager.aiEnabled
        z: 6
        Column {
            id: localActions
            anchors.centerIn: parent
            width: parent.width - 24
            spacing: 8
            Text {
                text: gameManager.sandbox ? (gameManager.currentSaveSlot > 0 ? qsTr("Sandbox · Slot %1").arg(gameManager.currentSaveSlot) : qsTr("Sandbox · Unsaved")) : gameManager.aiDifficulty + " AI · Slot " + gameManager.currentSaveSlot
                color: "#f3ede1"
                font.bold: true
            }
            Button {
                width: localActions.width
                text: gameManager.aiEnabled ? "Undo (" + gameManager.undoRemaining + ")" : "Undo"
                enabled: gameManager.canUndo
                onClicked: gameManager.undo()
            }
            Button {
                width: localActions.width
                text: "Redo"
                enabled: gameManager.canRedo
                onClicked: gameManager.redo()
            }
            Button {
                width: localActions.width
                text: qsTr("Reset")
                visible: gameManager.sandbox
                onClicked: resetDialog.open()
            }
            Button {
                width: localActions.width
                text: qsTr("Save")
                onClicked: {
                    gameManager.refreshSaves();
                    slotDialog.loading = false;
                    slotSelector.currentIndex = Math.max(0, gameManager.currentSaveSlot - 1);
                    slotDialog.open();
                }
            }
            Button {
                width: localActions.width
                text: qsTr("Load")
                onClicked: {
                    gameManager.refreshSaves();
                    slotDialog.loading = true;
                    const games = gameManager.sandbox ? gameManager.sandboxSavedGames : gameManager.savedGames;
                    const first = games.findIndex(function (game) {
                        return game.saved;
                    });
                    slotSelector.currentIndex = gameManager.currentSaveSlot > 0 ? gameManager.currentSaveSlot - 1 : Math.max(0, first);
                    slotDialog.open();
                }
            }
            Button {
                width: localActions.width
                text: qsTr("Exit")
                onClicked: {
                    gameManager.exitLocalGame();
                    stack.pop();
                }
            }
            Text {
                id: saveStatus
                text: ""
                color: "#f3ede1"
            }
        }
    }
    Dialog {
        id: resetDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("Reset game?")
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: gameManager.resetGame()
    }
    SaveGameDialog {
        id: slotDialog
        objectName: "slotDialog"
        property bool loading: false
        title: loading ? qsTr("Load game · 20 slots") : qsTr("Save game · Select a slot")
        actionText: loading ? qsTr("Load") : qsTr("Save")
        canAccept: slotSelector.canAccept
        onOpened: slotSelector.selectAvailable()
        contentItem: SaveSlotTable {
            id: slotSelector
            objectName: "slotSelector"
            loading: slotDialog.loading
            model: gameManager.sandbox ? gameManager.sandboxSavedGames : gameManager.savedGames
        }
        onAccepted: {
            if (loading) {
                if (gameManager.sandbox)
                    gameManager.loadSandbox(slotSelector.currentValue);
                else
                    gameManager.loadAIGame(slotSelector.currentValue);
            } else if (gameManager.saveGame(slotSelector.currentValue))
                saveStatus.text = qsTr("Saved");
        }
    }

    Button {
        id: passButton
        anchors.right: parent.right
        anchors.bottom: userInfo.top
        anchors.bottomMargin: 112
        anchors.rightMargin: 20
        text: qsTr("Pass")
        enabled: gameManager.canAct
        onClicked: gameManager.pass_turn()
    }

    Button {
        id: fastModeButton
        objectName: "fastModeButton"
        anchors.right: passButton.left
        anchors.verticalCenter: passButton.verticalCenter
        anchors.rightMargin: 12
        text: boardScreen.previewEnabled ? qsTr("Fast mode: OFF") : qsTr("Fast mode: ON")
        highlighted: !boardScreen.previewEnabled
        onClicked: boardScreen.previewEnabled = !boardScreen.previewEnabled
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Turn on/off preview mode")
    }

    Button {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
        text: qsTr("Surrender")
        visible: gameManager.online || gameManager.aiEnabled
        enabled: !gameManager.finished && !gameManager.pending
        onClicked: concedeDialog.open()
    }
    Dialog {
        id: concedeDialog
        anchors.centerIn: parent
        title: qsTr("Are you gay?")
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
        visible: (gameManager.online || gameManager.aiEnabled || gameManager.sandbox) && gameManager.finished
        Column {
            id: resultColumn
            anchors.centerIn: parent
            width: parent.width - 40
            spacing: 14
            Text {
                width: parent.width
                text: gameManager.sandbox ? (boardScreen.battleState.winnerIndex === 0 ? qsTr("Black wins") : qsTr("White wins")) : boardScreen.battleState.winnerIndex === gameManager.playerIndex ? qsTr("You won") : boardScreen.battleState.winnerIndex >= 0 ? qsTr("You lost") : qsTr("Game ended")
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 24
            }
            Text {
                width: parent.width
                text: gameManager.resultMessage
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: {
                    var score = boardScreen.scoring;
                    return score.blackPoints === undefined ? "" : qsTr("Japanese score (provisional)") + "\n" + qsTr("Black: %1 territory + %2 prisoners = %3 points").arg(score.blackTerritory).arg(score.blackPrisoners).arg(score.blackPoints) + "\n" + qsTr("White: %1 territory + %2 prisoners + %3 komi = %4 points").arg(score.whiteTerritory).arg(score.whitePrisoners).arg(score.komi).arg(score.whitePoints) + "\n" + qsTr("Difference: %1").arg(score.difference);
                }
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Back to menu")
                onClicked: {
                    gameManager.exitLocalGame();
                    stack.pop();
                }
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
        currentPoints: boardScreen.pointsForColor(1 - boardScreen.localPlayerColor)
        finalScoring: boardScreen.scoring.phase === "final"
        showServerPoints: gameManager.online && boardScreen.serverScoring.blackPoints !== undefined && boardScreen.serverScoring.whitePoints !== undefined
        serverPoints: (boardScreen.localPlayerColor === 0 ? boardScreen.serverScoring.whitePoints : boardScreen.serverScoring.blackPoints) ?? 0
        scoresMatch: boardScreen.scoringComparison.status === "matched"
        // provisionalPoints: boardScreen.scoring.provisional !== false
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
        currentPoints: boardScreen.pointsForColor(boardScreen.localPlayerColor)
        finalScoring: boardScreen.scoring.phase === "final"
        showServerPoints: gameManager.online && boardScreen.serverScoring.blackPoints !== undefined && boardScreen.serverScoring.whitePoints !== undefined
        serverPoints: (boardScreen.localPlayerColor === 0 ? boardScreen.serverScoring.blackPoints : boardScreen.serverScoring.whitePoints) ?? 0
        scoresMatch: boardScreen.scoringComparison.status === "matched"
        // provisionalPoints: boardScreen.scoring.provisional !== false
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
