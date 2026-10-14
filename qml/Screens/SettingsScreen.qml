import QtQuick
import QtQuick.Controls
import "../Components/controls"
import "../Singletons"
import GoGame

Rectangle {
    implicitWidth: 1280
    implicitHeight: 720
    color: "black"

    // BACKGROUND
    Item {
        id: background
        anchors.fill: parent

        property url currentSource: AssetManager.url("Background" + AssetManager.currentBackground)
        property bool showA: true

        onCurrentSourceChanged: {
            if (showA)
                imgB.source = currentSource;
            else
                imgA.source = currentSource;
            showA = !showA;
        }

        Component.onCompleted: imgA.source = currentSource

        Image {
            id: imgA
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            opacity: background.showA ? 1 : 0
            Behavior on opacity {
                NumberAnimation {
                    duration: 600
                    easing.type: Easing.InOutQuad
                }
            }
        }

        Image {
            id: imgB
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            opacity: background.showA ? 0 : 1
            Behavior on opacity {
                NumberAnimation {
                    duration: 600
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }

    /*Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: background.visible ? 0.35 : 0
    }*/

    // MAIN SETTINGS
    ScrollView {
        id: settingsScroll

        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        Column {
            id: settingsButtonsColumn

            width: settingsScroll.availableWidth
            spacing: 15

            Item {
                width: parent.width
                height: backgroundButtonsRow.implicitHeight

                Row {
                    id: backgroundButtonsRow

                    anchors.centerIn: parent
                    spacing: 15

                    Repeater {
                        model: 3

                        delegate: GameButton {
                            required property int index

                            opacity: AssetManager.currentBackground === index + 1 ? 1.0 : 0.6

                            width: 200
                            height: 60

                            text: qsTr("Background %1").arg(index + 1)

                            onClicked: {
                                AssetManager.currentBackground = index + 1;
                            }
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: stoneSkinsButtonsRow.implicitHeight

                Row {
                    id: stoneSkinsButtonsRow
                    anchors.centerIn: parent
                    spacing: 15

                    Repeater {
                        model: 2

                        delegate: GameButton {
                            id: stoneSkinButton

                            required property int index

                            width: 200
                            height: 60

                            opacity: AssetManager.currentStoneSkin === index + 1 ? 1.0 : 0.65

                            onClicked: {
                                AssetManager.currentStoneSkin = index + 1;
                            }

                            Item {
                                Row {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    spacing: 8
                                    height: 64

                                    Image {
                                        width: 128
                                        height: 128
                                        source: AssetManager.url("StoneSkin" + (stoneSkinButton.index + 1) + "Black")
                                        sourceSize: Qt.size(512, 512)
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                        cache: true
                                    }

                                    Image {
                                        width: 128
                                        height: 128
                                        source: AssetManager.url("StoneSkin" + (stoneSkinButton.index + 1) + "White")
                                        sourceSize: Qt.size(512, 512)
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                        cache: true
                                    }

                                    Text {
                                        width: 96
                                        height: 30
                                        text: qsTr("Skin %1").arg(stoneSkinButton.index + 1)
                                        color: stoneSkinButton.palette.buttonText
                                        font: stoneSkinButton.font
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    GameButton {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 20
        anchors.leftMargin: 20

        width: 200
        height: 60
        text: qsTr("Back")

        onClicked: {
            stack.pop();
        }
    }
}
