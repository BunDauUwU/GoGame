import QtQuick

Rectangle {
    id: root
    property var clockState: ({})
    property double elapsedMs: 0
    property bool active: false
    property bool finished: false
    property string playerLabel: ""
    readonly property var display: projectClock()
    function projectClock() {
        var main = clockState.mainRemainingMs === undefined ? 600000 : clockState.mainRemainingMs;
        var period = clockState.periodRemainingMs === undefined ? 20000 : clockState.periodRemainingMs;
        var duration = clockState.periodDurationMs || 20000;
        var count = clockState.periodsRemaining === undefined ? 3 : clockState.periodsRemaining;
        var elapsed = active && !finished ? Math.max(0, elapsedMs) : 0;
        var used = Math.min(main, elapsed);
        main -= used;
        elapsed -= used;
        if (main <= 0) {
            while (count > 0 && elapsed >= period) {
                elapsed -= period;
                count--;
                period = count > 0 ? duration : 0;
            }
            period = count > 0 ? Math.max(0, period - elapsed) : 0;
        }
        return { main: main, period: period, count: count };
    }
    function formatTime(ms) {
        var seconds = Math.max(0, Math.ceil(ms / 1000));
        return Math.floor(seconds / 60) + ":" + (seconds % 60 < 10 ? "0" : "") + seconds % 60;
    }
    implicitWidth: 260
    implicitHeight: 84
    radius: 12
    color: "#ed252c35"
    border.color: active && !finished ? "#e5c07b" : "#8d806d"
    border.width: active && !finished ? 2 : 1
    Column {
        anchors.centerIn: parent
        spacing: 3
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.playerLabel + (root.active && !root.finished ? qsTr(" · Đang đi") : "")
            color: "#d7c7ad"
            font.pixelSize: 13
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.display.main > 0 ? root.formatTime(root.display.main)
                  : Math.ceil(root.display.period / 1000) + qsTr(" giây")
            color: root.display.main <= 0 && root.display.period <= 5000 ? "#ff8877" : "#f3ede1"
            font.pixelSize: 26
            font.bold: true
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.display.main > 0 ? qsTr("Thời gian chính · Byo-yomi 3 × 20s")
                  : qsTr("Byo-yomi · Còn ") + root.display.count + qsTr(" lần")
            color: "#d7c7ad"
            font.pixelSize: 12
        }
    }
}
