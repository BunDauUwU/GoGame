import QtQuick
import QtQuick.Controls

Item {

    anchors.fill: parent

    StackView {
        id: stack

        anchors.fill: parent

        initialItem: "../Screens/LoginScreen.qml"
    }
}
