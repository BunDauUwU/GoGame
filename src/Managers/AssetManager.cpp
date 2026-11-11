// src/Managers/AssetManager.cpp
#include "AssetManager.h"

AssetManager::AssetManager(QObject* parent) : QObject(parent) {}

const QHash<QString, QString> AssetManager::paths = {
    {"AnnouncementFont", "fonts/street-fighter.otf"},
    {"Background1", "background/background1.png"},
    {"Background2", "background/background2.png"},
    {"Background3", "background/background3.png"},
    {"Black1", "stones/black1.png"},
    {"White1", "stones/white1.png"},
    {"Black2", "stones/black2.png"},
    {"White2", "stones/white2.png"},
    {"PlaceSound", "sounds/place.wav"},
    {"ErrorSound", "sounds/error.mp3"},
    {"BackgroundSong1", "sounds/background1.mp3"},
    {"BackgroundSong2", "sounds/background2.opus"},
    {"BackgroundSong3", "sounds/background3.mp3"},
    {"BoardTexture1", "board/variant1.jpg"},
    {"BoardTexture2", "board/variant2.jpg"},
    {"BoardTexture3", "board/variant3.jpg"},
    {"BoardTexture4", "board/variant4.jpg"},
    {"BoardTexture5", "board/variant5.jpg"},
    {"BoardTexture6", "board/variant6.jpg"},
    {"CaptureSound", "sounds/capture1.mp3"},
    {"KoSound", "sounds/ko1.mp3"},
    {"WinSound", "sounds/win.mp3"},
};

int AssetManager::currentBackground() const { return m_currentBackground; }

void AssetManager::setCurrentBackground(int index) {
    if (m_currentBackground == index) return;

    m_currentBackground = index;
    emit currentBackgroundChanged();
}

int AssetManager::currentStoneSkin() const { return m_currentStoneSkin; }

void AssetManager::setCurrentStoneSkin(int index) {
    if (m_currentStoneSkin == index) return;

    m_currentStoneSkin = index;
    emit currentStoneSkinChanged();
}

int AssetManager::currentBoardTexture() const { return m_currentBoardTexture; }

void AssetManager::setCurrentBoardTexture(int index) {
    if (m_currentBoardTexture == index) return;

    m_currentBoardTexture = index;
    emit currentBoardTextureChanged();
}

int AssetManager::currentBackgroundSong() const { return m_currentBackgroundSong; }

void AssetManager::setCurrentBackgroundSong(int index) {
    if (m_currentBackgroundSong == index) return;

    m_currentBackgroundSong = index;
    emit currentBackgroundSongChanged();
}

double AssetManager::bgmVolume() const { return m_bgmVolume; }

void AssetManager::setBgmVolume(double volume) {
    if (qFuzzyCompare(m_bgmVolume, volume)) return;

    m_bgmVolume = volume;
    emit bgmVolumeChanged();
}

double AssetManager::sfxVolume() const { return m_sfxVolume; }

void AssetManager::setSfxVolume(double volume) {
    if (qFuzzyCompare(m_sfxVolume, volume)) return;

    m_sfxVolume = volume;
    emit sfxVolumeChanged();
}

QUrl AssetManager::url(const QString& name) const {
    if (!paths.contains(name)) return {};

    return QUrl("qrc:/GoGame/assets/" + paths.value(name));
}
