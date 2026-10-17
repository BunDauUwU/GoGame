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
  Q_PROPERTY(int currentBackgroundSong READ currentBackgroundSong WRITE
                 setCurrentBackgroundSong NOTIFY currentBackgroundSongChanged)
  Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)

public:
  explicit AssetManager(QObject *parent = nullptr);
  int currentBackground() const;
  void setCurrentBackground(int index);

  int currentStoneSkin() const;
  void setCurrentStoneSkin(int index);

  int currentBackgroundSong() const;
  void setCurrentBackgroundSong(int index);

  double volume() const;
  void setVolume(double volume);

  Q_INVOKABLE QUrl url(const QString &name) const;

signals:
  void currentBackgroundChanged();
  void currentStoneSkinChanged();
  void currentBackgroundSongChanged();
  void volumeChanged();

private:
  static const QHash<QString, QString> paths;
  int m_currentBackground = 1;
  int m_currentStoneSkin = 1;
  int m_currentBackgroundSong = 1;
  double m_volume = 1.0;
};
