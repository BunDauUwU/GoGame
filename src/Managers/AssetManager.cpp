// src/Managers/AssetManager.cpp
#include "AssetManager.h"



AssetManager::AssetManager(QObject *parent)
    : QObject(parent) {}

const QHash<QString, QString> AssetManager::paths = {
    {"Background1", "background/background1.png"},
    {"Background2", "background/background2.png"},
    {"Background3", "background/background3.png"},
};

QUrl AssetManager::url(const QString &name) const {
    if (!paths.contains(name))
        return {};

    return QUrl("qrc:/GoGame/assets/" + paths.value(name));
}