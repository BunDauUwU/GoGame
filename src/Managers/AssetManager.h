// src/Managers/AssetManager.h
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>
#include <QHash>
#include <QString>
#include <QUrl>
#include <qqmlintegration.h>

class AssetManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit AssetManager(QObject *parent = nullptr);

    Q_INVOKABLE QUrl url(const QString &name) const;

private:
    static const QHash<QString, QString> paths;
};
