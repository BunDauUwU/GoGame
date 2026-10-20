import QtQuick
import QtQuick.Controls

Dialog {
    id: root
    property string actionText: qsTr("Load")
    property bool canAccept: false
    anchors.centerIn: parent
    modal: true
    width: Math.min(1000, parent ? parent.width - 40 : 1000)
    height: Math.min(650, parent ? parent.height - 40 : 650)
    padding: 18
    background: Rectangle {
        color: "#202733"
        radius: 12
        border.color: "#b69a6a"
        border.width: 1
    }
    header: Label {
        text: root.title
        color: "#ead6b1"
        font.pixelSize: 24
        font.bold: true
        padding: 22
        bottomPadding: 14
    }
    footer: DialogButtonBox {
        background: Item {}
        padding: 16
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: root.actionText
            enabled: root.canAccept
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
