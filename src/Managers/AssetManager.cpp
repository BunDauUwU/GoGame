// src/Managers/AssetManager.cpp
#include "AssetManager.h"

AssetManager::AssetManager(QObject *parent) : QObject(parent) {}

const QHash<QString, QString> AssetManager::paths = {
    {"Background1", "background/background1.png"},
    {"Background2", "background/background2.png"},
    {"Background3", "background/background3.png"},
    {"Black1", "stones/black1.png"},
    {"White1", "stones/white1.png"},
    {"Black2", "stones/black2.png"},
    {"White2", "stones/white2.png"},
    {"PlaceSound", "sounds/place.wav"},
    {"BackgroundSong1", "sounds/background1.mp3"},
    {"BackgroundSong2", "sounds/background2.opus"},
};

int AssetManager::currentBackground() const { return m_currentBackground; }

void AssetManager::setCurrentBackground(int index) {
  if (m_currentBackground == index)
    return;

  m_currentBackground = index;
  emit currentBackgroundChanged();
}

int AssetManager::currentStoneSkin() const { return m_currentStoneSkin; }

void AssetManager::setCurrentStoneSkin(int index) {
  if (m_currentStoneSkin == index)
    return;

  m_currentStoneSkin = index;
  emit currentStoneSkinChanged();
}

int AssetManager::currentBackgroundSong() const {
  return m_currentBackgroundSong;
}

void AssetManager::setCurrentBackgroundSong(int index) {
  if (m_currentBackgroundSong == index)
    return;

  m_currentBackgroundSong = index;
  emit currentBackgroundSongChanged();
}

double AssetManager::bgmVolume() const { return m_bgmVolume; }

void AssetManager::setBgmVolume(double volume) {
  if (qFuzzyCompare(m_bgmVolume, volume))
    return;

  m_bgmVolume = volume;
  emit bgmVolumeChanged();
}

double AssetManager::sfxVolume() const { return m_sfxVolume; }

void AssetManager::setSfxVolume(double volume) {
  if (qFuzzyCompare(m_sfxVolume, volume))
    return;

  m_sfxVolume = volume;
  emit sfxVolumeChanged();
}

QUrl AssetManager::url(const QString &name) const {
  if (!paths.contains(name))
    return {};

  return QUrl("qrc:/GoGame/assets/" + paths.value(name));
}