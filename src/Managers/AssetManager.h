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

public:
  explicit AssetManager(QObject *parent = nullptr);
  int currentBackground() const;
  void setCurrentBackground(int index);

  Q_INVOKABLE QUrl url(const QString &name) const;

signals:
  void currentBackgroundChanged();

private:
  static const QHash<QString, QString> paths;
  int m_currentBackground = 1;
};
