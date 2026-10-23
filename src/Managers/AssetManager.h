// src/Managers/AssetManager.h
#pragma once

#include <QHash>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>
#include <qqmlintegration.h>

class AssetManager : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(int currentBackground READ currentBackground WRITE
                 setCurrentBackground NOTIFY currentBackgroundChanged)
  Q_PROPERTY(int currentStoneSkin READ currentStoneSkin WRITE
                 setCurrentStoneSkin NOTIFY currentStoneSkinChanged)
  Q_PROPERTY(int currentBoardTexture READ currentBoardTexture WRITE
                 setCurrentBoardTexture NOTIFY currentBoardTextureChanged)
  Q_PROPERTY(int currentBackgroundSong READ currentBackgroundSong WRITE
                 setCurrentBackgroundSong NOTIFY currentBackgroundSongChanged)
  Q_PROPERTY(double bgmVolume READ bgmVolume WRITE setBgmVolume NOTIFY
                 bgmVolumeChanged)
  Q_PROPERTY(double sfxVolume READ sfxVolume WRITE setSfxVolume NOTIFY
                 sfxVolumeChanged)

public:
  explicit AssetManager(QObject *parent = nullptr);
  int currentBackground() const;
  void setCurrentBackground(int index);

  int currentStoneSkin() const;
  void setCurrentStoneSkin(int index);

  int currentBoardTexture() const;
  void setCurrentBoardTexture(int index);

  int currentBackgroundSong() const;
  void setCurrentBackgroundSong(int index);

  double bgmVolume() const;
  void setBgmVolume(double volume);

  double sfxVolume() const;
  void setSfxVolume(double volume);

  Q_INVOKABLE QUrl url(const QString &name) const;

signals:
  void currentBackgroundChanged();
  void currentStoneSkinChanged();
  void currentBoardTextureChanged();
  void currentBackgroundSongChanged();
  void bgmVolumeChanged();
  void sfxVolumeChanged();

private:
  static const QHash<QString, QString> paths;
  int m_currentBackground = 1;
  int m_currentStoneSkin = 1;
  int m_currentBoardTexture = 1;
  int m_currentBackgroundSong = 1;
  double m_bgmVolume = 1.0;
  double m_sfxVolume = 1.0;
};
