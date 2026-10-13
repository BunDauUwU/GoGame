import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components/controls"
import "../Components/layout"
import "../Singletons"
import "../Core"
import GoGame

Rectangle {
    implicitWidth: 1280
    implicitHeight: 720
    color: "black"

    Item {
        id: background
        anchors.fill: parent
        
        Image {
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            source: AssetManager.url("Background" + AppState.backgroundIndex)
        }
    }
}
