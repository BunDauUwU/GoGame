#pragma once

#include <QObject>
#include <QJsonObject>
#include <QWebSocketServer>

class QWebSocket;

class Server : public QObject {
public:
    explicit Server(const QString &dataFile, QObject *parent = nullptr);
    bool start(quint16 port, QString &error);

    void read(QWebSocket *client, const QString &text);
    void handler(QWebSocket *client, const QJsonObject &message);
    qint64 send(QWebSocket *client, const QJsonObject &message);

    static QString jsonToString(const QJsonObject &json);
    static bool stringToJson(const QString &text, QJsonObject &json, QString &error);
    static QJsonObject envelope(const QString &type, const QJsonObject &payload = {});

    bool readData(QJsonObject &data, QString &error) const;
    bool writeData(const QJsonObject &data, QString &error) const;
    QString dataFile() const { return m_dataFile; }

private:
    void acceptConnection();
    void fail(QWebSocket *client, const QString &code, const QString &message);

    QString m_dataFile;
    QWebSocketServer m_server;
};
