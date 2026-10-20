import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property var model: []
    property bool loading: false
    property int currentIndex: -1
    readonly property var selectedGame: currentIndex >= 0 && currentIndex < model.length ? model[currentIndex] : null
    readonly property int currentValue: selectedGame ? selectedGame.slot : 0
    readonly property bool canAccept: selectedGame !== null && (!loading || selectedGame.saved)
    readonly property int slotWidth: width < 600 ? 54 : 82
    readonly property int timeWidth: Math.floor(width * 0.40)
    color: "#171e28"
    radius: 8
    border.color: "#625544"
    implicitWidth: 860
    implicitHeight: 430

    function selectAvailable() {
        if (canAccept)
            return;
        currentIndex = -1;
        for (let i = 0; i < model.length; ++i) {
            if (!loading || model[i].saved) {
                currentIndex = i;
                break;
            }
        }
    }

    function savedTime(game) {
        if (!game.saved)
            return "—";
        const date = new Date(game.savedAt);
        return isNaN(date.getTime()) ? qsTr("Unknown") : Qt.formatDateTime(date, "dd/MM/yyyy HH:mm:ss");
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            color: "#2b3038"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 14
                Label {
                    text: qsTr("SLOT")
                    color: "#d8c29b"
                    font.bold: true
                    Layout.preferredWidth: root.slotWidth
                }
                Label {
                    text: qsTr("GAME MODE")
                    color: "#d8c29b"
                    font.bold: true
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("SAVED AT")
                    color: "#d8c29b"
                    font.bold: true
                    Layout.preferredWidth: root.timeWidth
                }
                Label {
                    text: qsTr("STATUS")
                    color: "#d8c29b"
                    font.bold: true
                    Layout.preferredWidth: 76
                }
            }
        }
        ListView {
            id: rows
            objectName: "saveSlotRows"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.model
            currentIndex: root.currentIndex
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
            onCurrentIndexChanged: {
                if (activeFocus)
                    root.currentIndex = currentIndex;
            }
            delegate: ItemDelegate {
                id: row
                required property var modelData
                required property int index
                width: rows.width
                height: 58
                enabled: !root.loading || modelData.saved
                hoverEnabled: true
                onClicked: {
                    root.currentIndex = index;
                    rows.forceActiveFocus();
                }
                background: Rectangle {
                    color: root.currentIndex === row.index ? "#3e3a32" : row.hovered ? "#29313d" : row.index % 2 === 0 ? "#1d2530" : "#19212b"
                    border.width: root.currentIndex === row.index ? 1 : 0
                    border.color: "#c6a773"
                    Rectangle {
                        width: 3
                        height: parent.height
                        color: "#d9bb84"
                        visible: root.currentIndex === row.index
                    }
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: "#303743"
                    }
                }
                contentItem: RowLayout {
                    spacing: 14
                    Label {
                        text: (root.currentIndex === row.index ? "›  " : "   ") + String(row.modelData.slot).padStart(2, "0")
                        color: "#e7d0a4"
                        font.bold: true
                        Layout.preferredWidth: root.slotWidth
                    }
                    Label {
                        text: row.modelData.saved ? (row.modelData.mode === "Sandbox" ? qsTr("Sandbox") : qsTr("%1 AI").arg(row.modelData.mode)) : qsTr("Empty slot")
                        color: row.modelData.saved ? "#eee7db" : "#828995"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: root.savedTime(row.modelData)
                        color: row.modelData.saved ? "#bec5cf" : "#68717d"
                        elide: Text.ElideRight
                        Layout.preferredWidth: root.timeWidth
                    }
                    Label {
                        text: row.modelData.saved ? qsTr("Saved") : qsTr("Empty")
                        color: row.modelData.saved ? "#a7c6ad" : "#828995"
                        Layout.preferredWidth: 76
                    }
                }
            }
            onCountChanged: Qt.callLater(root.selectAvailable)
        }
    }
    onCurrentIndexChanged: rows.positionViewAtIndex(currentIndex, ListView.Contain)
}
