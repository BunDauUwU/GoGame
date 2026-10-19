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
                height: 60

                Row {
                    id: stoneSkinsButtonsRow

                    anchors.centerIn: parent
                    spacing: 15

                    Repeater {
                        model: 2

                        delegate: GameButton {
                            required property int index

                            width: 200
                            height: 60

                            text: qsTr("Stone Skin %1").arg(index + 1)

                            opacity: AssetManager.currentStoneSkin === index + 1 ? 1.0 : 0.45

                            Behavior on opacity {
                                NumberAnimation {
                                    duration: 180
                                    easing.type: Easing.InOutQuad
                                }
                            }

                            onClicked: {
                                AssetManager.currentStoneSkin = index + 1;
                            }
                        }
                    }
                }
            }

            Item {
                id: stoneSkinPreview

                width: parent.width
                height: 200

                Column {
                    anchors.centerIn: parent
                    spacing: 12

                    Text {
                        width: 340

                        text: qsTr("Stone Skin %1").arg(AssetManager.currentStoneSkin)

                        color: "black"
                        font.pixelSize: 22
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 50

                        Image {
                            width: 140
                            height: 140

                            source: AssetManager.url("Black" + AssetManager.currentStoneSkin)

                            fillMode: Image.PreserveAspectFit
                            sourceSize: Qt.size(512, 512)
                            asynchronous: true
                            smooth: true
                            mipmap: true
                            cache: true
                        }

                        Image {
                            width: 140
                            height: 140

                            source: AssetManager.url("White" + AssetManager.currentStoneSkin)

                            fillMode: Image.PreserveAspectFit
                            sourceSize: Qt.size(512, 512)
                            asynchronous: true
                            smooth: true
                            mipmap: true
                            cache: true
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: backgroundSongButtonsRow.implicitHeight

                Row {
                    id: backgroundSongButtonsRow

                    anchors.centerIn: parent
                    spacing: 15

                    Repeater {
                        model: 3

                        delegate: GameButton {
                            required property int index

                            opacity: AssetManager.currentBackgroundSong === index + 1 ? 1.0 : 0.6

                            width: 200
                            height: 60

                            text: qsTr("Background Song %1").arg(index + 1)

                            onClicked: {
                                AssetManager.currentBackgroundSong = index + 1;
                            }
                        }
                    }
                }
            }

            Item {
                id: bgmVolumeSettings

                width: parent.width
                height: 100

                Column {
                    anchors.centerIn: parent
                    spacing: 12

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "BGM Volume"
                        color: "black"
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Slider {
                        anchors.horizontalCenter: parent.horizontalCenter
                        from: 0.0
                        to: 1.0
                        value: AssetManager.bgmVolume
                        onMoved: AssetManager.bgmVolume = value
                    }
                }
            }

            Item {
                id: sfxVolumeSettings

                width: parent.width
                height: 100

                Column {
                    anchors.centerIn: parent
                    spacing: 12

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "SFX Volume"
                        color: "black"
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Slider {
                        anchors.horizontalCenter: parent.horizontalCenter
                        from: 0.0
                        to: 1.0
                        value: AssetManager.sfxVolume
                        onMoved: AssetManager.sfxVolume = value
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
