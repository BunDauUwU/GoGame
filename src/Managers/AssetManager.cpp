// src/Managers/AssetManager.cpp
#include "AssetManager.h"



AssetManager::AssetManager(QObject *parent)
    : QObject(parent) {}

const QHash<QString, QString> AssetManager::paths = {
    {"Background1", "background/background1.png"},
    {"Background2", "background/background2.png"},
    {"Background3", "background/background3.png"},

    {"StoneSkin1Black", "stones/black1.png"},
    {"StoneSkin1White", "stones/white1.png"},
    {"StoneSkin2Black", "stones/black2.png"},
    {"StoneSkin2White", "stones/white2.png"},
};

int AssetManager::currentBackground() const
{
    return m_currentBackground;
}

void AssetManager::setCurrentBackground(int index)
{
    if (m_currentBackground == index)
        return;

    m_currentBackground = index;
    emit currentBackgroundChanged();
}

int AssetManager::currentStoneSkin() const
{
    return m_currentStoneSkin;
}

void AssetManager::setCurrentStoneSkin(int index)
{
    if (m_currentStoneSkin == index)
        return;

    m_currentStoneSkin = index;
    emit currentStoneSkinChanged();
}

QUrl AssetManager::url(const QString &name) const {
    if (!paths.contains(name))
        return {};

    return QUrl("qrc:/GoGame/assets/" + paths.value(name));
}