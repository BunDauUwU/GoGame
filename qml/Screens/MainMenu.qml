import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components/controls"
import "../Core"
import GoGame

Rectangle {
    id: mainMenuRoot
    implicitWidth: 1280
    implicitHeight: 720
    property string matchmakingError: ""

    // ==========================================
    // LOGIC & CONNECTIONS (Đặt ngoài hệ thống bố cục)
    // ==========================================
    Connections {
        target: matchmakingManager

        function onMatchFound(matchId, playerIndex, opponentName) {
            console.log("Match found:", matchId, playerIndex, opponentName);
            stack.push("./MatchScreen.qml", {
                localPlayerName: authManager.username,
                opponentPlayerName: opponentName,
                localPlayerColor: playerIndex
            });
        }

        function onMatchmakingError(message) {
            mainMenuRoot.matchmakingError = message;
        }
    }

    Connections {
        target: gameManager
        function onGameError(message) {
            mainMenuRoot.matchmakingError = message;
        }
    }

    Connections {
        target: networkClient

        function onErrorOccurred(error) {
            console.log("Network error:", error);
        }
    }

    // ==========================================
    // BACKGROUND & VISUAL OVERLAY
    // ==========================================
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#27374D"
            }
            GradientStop {
                position: 1
                color: "#0F172A"
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.35
    }

    Popup {
        id: selectionMenu

        parent: Overlay.overlay
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        modal: true
        focus: true
        width: 280
        padding: 12
        background: Rectangle {
            color: "transparent"
        }

        enter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 0.0
                    to: 1.0
                    duration: 180
                    easing.type: Easing.OutCubic
                }

                NumberAnimation {
                    property: "scale"
                    from: 0.85
                    to: 1.0
                    duration: 220
                    easing.type: Easing.OutBack
                }
            }
        }

        exit: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 1.0
                    to: 0.0
                    duration: 130
                    easing.type: Easing.InCubic
                }

                NumberAnimation {
                    property: "scale"
                    from: 1.0
                    to: 0.9
                    duration: 130
                    easing.type: Easing.InCubic
                }
            }
        }

        contentItem: GamePanel {
            Layout.fillWidth: true
            implicitHeight: 240
            radius: Radius.xl

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Spacing.xl
                spacing: Spacing.md

                Repeater {
                    model: ["Easy", "Medium", "Hard"]

                    delegate: GameButton {
                        required property string modelData
                        required property int index

                        text: modelData
                        Layout.fillWidth: true
                        enabled: index === 1

                        onClicked: {
                            console.log("Selected:", modelData);

                            if (gameManager.savedAISlots.length >= 20) {
                                newAIDialog.open();
                                selectionMenu.close();
                                return;
                            }
                            if (!gameManager.startMediumGame(0))
                                return;
                            AppState.difficultyLevel = index;
                            AppState.playingAgainstAI = true;
                            stack.push("./MatchScreen.qml", {
                                localPlayerName: authManager.username || "Player",
                                opponentPlayerName: "Medium AI"
                            });

                            selectionMenu.close();
                        }
                    }
                }
            }
        }
    }

    // ==========================================
    // TITLE CONTAINER
    // ==========================================
    Column {
        id: titleColumn
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 90
        spacing: 8

        Text {
            text: "✦ ✦ ✦"
            font.pixelSize: 18
            color: "#E5C07B"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "GoGame"
            font.pixelSize: 64
            font.bold: true
            color: "white"
            style: Text.Outline
            styleColor: "#202020"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "The Go Adventure"
            font.pixelSize: 22
            color: "#D8D8D8"
            font.italic: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "✦ ✦ ✦"
            font.pixelSize: 18
            color: "#E5C07B"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    // ==========================================
    // PLAYER INFO (Top-Left)
    // ==========================================
    Column {
        id: playerInfoColumn
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        spacing: 8

        Button {
            id: connectionStatusButton

            text: networkClient.connected ? "● Server Connected" : "○ Offline Mode (Click here to retry connection)"

            enabled: !networkClient.connected
            hoverEnabled: true

            scale: pressed ? 0.94 : 1.0

            Behavior on scale {
                SpringAnimation {
                    spring: 3
                    damping: 0.2
                    mass: 0.5
                }
            }

            onClicked: {
                networkClient.disconnectFromServer();
                reconnectTimer.restart();
            }

            HoverHandler {
                cursorShape: connectionStatusButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            }

            contentItem: Text {
                text: connectionStatusButton.text
                color: networkClient.connected ? "#4ADE80" : "#F87171"
                font.pixelSize: 13
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: "transparent"
            }
        }

        Text {
            text: "Player: " + (authManager.username.length > 0 ? authManager.username : "Guest")
            color: "#F8E3A6"
            font.pixelSize: 15
            font.bold: true
        }
    }

    // ==========================================
    // MENU NAVIGATION BUTTONS (Bottom-Right)
    // ==========================================
    Column {
        id: menuButtonsColumn
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: 20
        anchors.rightMargin: 20
        spacing: 15

        Text {
            text: matchmakingManager.statusText
            color: "#E2E8F0"
            font.pixelSize: 15
            font.italic: true
            visible: matchmakingManager.searching
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: mainMenuRoot.matchmakingError
            color: "#F87171"
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            width: 240
            visible: text.length > 0
        }

        GameButton {
            height: 60
            width: 400
            text: "Play against AI"
            onClicked: {
                console.log("Matchmaking action triggered");
                selectionMenu.open();
            }
        }

        GameButton {
            height: 48
            width: 400
            text: "Sandbox"
            onClicked: {
                if (gameManager.startSandbox())
                    stack.push("./MatchScreen.qml", {
                        localPlayerName: "Black",
                        opponentPlayerName: "White",
                        localPlayerColor: 0
                    });
            }
        }
        GameButton {
            height: 48
            width: 400
            text: "Load game"
            onClicked: {
                gameManager.refreshSaves();
                loadAIDialog.open();
            }
        }

        GameButton {
            height: 60
            width: 400
            enabled: networkClient.connected
            text: !networkClient.connected ? "Multiplayer unavailable" : matchmakingManager.searching ? "Cancel Match" : "Find Match"
            onClicked: {
                console.log("Matchmaking action triggered");
                if (matchmakingManager.searching) {
                    matchmakingManager.cancelMatchmaking();
                } else {
                    mainMenuRoot.matchmakingError = "";
                    matchmakingManager.startMatchmaking("Go");
                }
            }
        }

        GameButton {
            height: 60
            width: 400
            text: qsTr("Settings")
            onClicked: {
                stack.push("./SettingsScreen.qml");
            }
        }

        GameButton {
            height: 60
            width: 400
            text: qsTr("Log Out")
            onClicked: {
                authManager.logout();
                stack.pop();
            }
        }
    }

    Dialog {
        id: newAIDialog
        anchors.centerIn: parent
        modal: true
        title: "Medium"
        standardButtons: Dialog.Ok | Dialog.Cancel
        ComboBox {
            id: newSlotSelector
            model: 20
            displayText: "Slot " + (currentIndex + 1)
        }
        onAccepted: {
            if (gameManager.startMediumGame(0, newSlotSelector.currentIndex + 1)) {
                stack.push("./MatchScreen.qml", {
                    localPlayerName: authManager.username || "Player",
                    opponentPlayerName: "Medium AI"
                });
            }
        }
    }

    Dialog {
        id: loadAIDialog
        anchors.centerIn: parent
        modal: true
        title: "Load AI battle | 20 slots"
        standardButtons: Dialog.Ok | Dialog.Cancel
        ComboBox {
            id: loadAISelector
            model: gameManager.savedAISlots
            displayText: currentIndex >= 0 ? "Slot " + currentText : "No saved AI battles"
        }
        onAccepted: {
            if (loadAISelector.currentIndex >= 0 && gameManager.loadAIGame(Number(loadAISelector.currentText))) {
                stack.push("./MatchScreen.qml", {
                    localPlayerName: authManager.username || "Player",
                    opponentPlayerName: "Medium AI"
                });
            }
        }
    }

    Timer {
        id: reconnectTimer
        interval: 250
        repeat: false
        onTriggered: networkClient.connectToServer(networkClient.serverUrl)
    }
}
