pragma Singleton
import QtQuick

QtObject {
    property int backgroundIndex: 1
    property bool playingAgainstAI: false
    property int difficultyLevel: 0 // 0 easy, 1 medium, 2 hard
}