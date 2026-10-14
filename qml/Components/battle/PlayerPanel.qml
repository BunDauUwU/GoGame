import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string playerName: "Some template on the internet"
    property real currentPoints: 0
    property real serverPoints: 0
    property bool showServerPoints: false
    property bool scoresMatch: true
    property bool isJapanRule: true
    property bool provisionalPoints: true
    property bool finalScoring: false
    property url avatarSource: "qrc:/images/avatar.png"
    property bool avatarOnRight: false
    property color frameColor: "#7f94a4"
    readonly property real unit: height / 90

    implicitWidth: 420
    implicitHeight: 90

    Canvas {
        id: frame
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        Connections {
            target: root
            function onAvatarOnRightChanged() {
                frame.requestPaint();
            }
            function onFrameColorChanged() {
                frame.requestPaint();
            }
        }

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            var u = root.unit;
            ctx.scale(u, u);
            var w = width / u;

            if (root.avatarOnRight) {
                ctx.translate(w, 0);
                ctx.scale(-1, 1);
            }

            function outline(inset) {
                ctx.beginPath();
                ctx.moveTo(35, 5 + inset);

                ctx.lineTo(w - 75, 5 + inset);

                ctx.lineTo(w - 75, 85 - inset);

                ctx.lineTo(35, 85 - inset);

                ctx.quadraticCurveTo(5 + inset, 85 - inset, 5 + inset, 55);
                ctx.lineTo(5 + inset, 35);
                ctx.quadraticCurveTo(5 + inset, 5 + inset, 35, 5 + inset);
                ctx.closePath();
            }

            outline(0);
            ctx.shadowColor = "#aa000000";
            ctx.shadowBlur = 8;
            ctx.shadowOffsetY = 4;

            var gradient = ctx.createLinearGradient(0, 0, 0, 90);
            gradient.addColorStop(0, "#161b22");
            gradient.addColorStop(0.5, "#0b0d11");
            gradient.addColorStop(1, "#11141a");
            ctx.fillStyle = gradient;
            ctx.fill();

            ctx.shadowBlur = 0;
            ctx.shadowOffsetY = 0;

            ctx.strokeStyle = root.frameColor;
            ctx.lineWidth = 3;
            ctx.stroke();

            outline(4);
            ctx.strokeStyle = "#384556";
            ctx.lineWidth = 1;
            ctx.stroke();
        }
    }

    Rectangle {
        id: avatarFrame
        x: root.avatarOnRight ? parent.width - width - 15 * root.unit : 15 * root.unit
        anchors.verticalCenter: parent.verticalCenter
        width: 70 * root.unit
        height: 70 * root.unit
        radius: width / 2
        color: "#0f1318"
        border.color: root.frameColor
        border.width: 2 * root.unit

        Image {
            anchors.fill: parent
            anchors.margins: 4 * root.unit
            source: root.avatarSource
            fillMode: Image.PreserveAspectCrop
            smooth: true

            property bool isRound: true
        }
    }

    Item {
        id: textContainer
        HoverHandler {
            id: pointsHover
        }
        ToolTip.visible: pointsHover.hovered
        ToolTip.text: root.finalScoring ? qsTr("Enclosed territory + prisoners; White includes komi. Dead stones and seki still need confirmation.") : root.provisionalPoints ? qsTr("Secured territory + prisoners; White includes komi. Open or unsettled regions are not counted.") : qsTr("Territory + prisoners; White includes komi.")
        x: root.avatarOnRight ? 90 * root.unit : 100 * root.unit
        width: parent.width - 160 * root.unit
        height: parent.height
        anchors.verticalCenter: parent.verticalCenter

        Text {
            id: nameText
            y: 16 * root.unit
            text: root.playerName
            width: parent.width
            elide: Text.ElideRight
            color: "#ffffff"
            font.family: "Arial"
            font.pixelSize: 20 * root.unit
            font.bold: true
        }

        Text {
            id: pointsLabel
            x: 5 * root.unit
            y: 56 * root.unit
            text: qsTr("Current points")
            color: root.frameColor
            font.family: "Arial"
            font.pixelSize: 14 * root.unit
            font.bold: true
        }

        Text {
            id: pointsValue
            anchors.left: pointsLabel.right
            anchors.leftMargin: 15 * root.unit
            anchors.bottom: pointsLabel.bottom
            text: Number(root.currentPoints).toLocaleString(Qt.locale(), "f", root.currentPoints % 1 === 0 ? 0 : 1)
            color: "#ffffff"
            font.family: "Consolas"
            font.pixelSize: 26 * root.unit
            font.bold: true
        }
        Text {
            y: 75 * root.unit
            width: parent.width
            visible: root.showServerPoints
            text: qsTr("Server: %1").arg(Number(root.serverPoints).toLocaleString(Qt.locale(), "f", root.serverPoints % 1 === 0 ? 0 : 1))
            color: root.scoresMatch ? root.frameColor : "#e0a46d"
            font.pixelSize: 11 * root.unit
            elide: Text.ElideRight
        }
    }
}
