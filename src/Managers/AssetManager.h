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

public:
  explicit AssetManager(QObject *parent = nullptr);
  int currentBackground() const;
  void setCurrentBackground(int index);
  int currentStoneSkin() const;
  void setCurrentStoneSkin(int index);

  Q_INVOKABLE QUrl url(const QString &name) const;

signals:
  void currentBackgroundChanged();
  void currentStoneSkinChanged();

private:
  static const QHash<QString, QString> paths;
  int m_currentBackground = 1;
  int m_currentStoneSkin = 1;
};
