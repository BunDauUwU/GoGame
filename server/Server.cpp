#include "Server.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QWebSocket>

Server::Server(const QString &dataFile, QObject *parent)
    : QObject(parent), m_dataFile(QFileInfo(dataFile).absoluteFilePath()),
      m_server(QStringLiteral("GoGameServer"), QWebSocketServer::NonSecureMode)
{
    connect(&m_server, &QWebSocketServer::newConnection, this, &Server::acceptConnection);
}

bool Server::start(quint16 port, QString &error)
{
    error.clear();
    if (port == 0) {
        error = QStringLiteral("Port must be between 1 and 65535");
        return false;
    }
    if (!QDir().mkpath(QFileInfo(m_dataFile).absolutePath())) {
        error = QStringLiteral("Cannot create storage directory");
        return false;
    }
    QJsonObject data;
    if ((!QFileInfo::exists(m_dataFile) && !writeData({}, error)) || !readData(data, error))
        return false;
    if (!m_server.listen(QHostAddress::LocalHost, port)) {
        error = m_server.errorString();
        return false;
    }
    return true;
}

void Server::acceptConnection()
{
    QWebSocket *client = m_server.nextPendingConnection();
    if (!client)
        return;
    client->setParent(&m_server);
    client->setMaxAllowedIncomingMessageSize(1024 * 1024);
    connect(client, &QWebSocket::disconnected, client, &QObject::deleteLater);
    connect(client, &QWebSocket::textMessageReceived, this,
            [this, client](const QString &text) { read(client, text); });
    connect(client, &QWebSocket::binaryMessageReceived, this,
            [this, client](const QByteArray &) {
        fail(client, "UnsupportedMessage", "Send JSON as a text message");
    });
}

QJsonObject Server::envelope(const QString &type, const QJsonObject &payload)
{
    return {{"type", type}, {"payload", payload}};
}

QString Server::jsonToString(const QJsonObject &json)
{
    return QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

bool Server::stringToJson(const QString &text, QJsonObject &json, QString &error)
{
    error.clear();
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("Message must be a valid JSON object");
        return false;
    }
    json = document.object();
    return true;
}

qint64 Server::send(QWebSocket *client, const QJsonObject &message)
{
    if (!client || client->state() != QAbstractSocket::ConnectedState)
        return -1;
    return client->sendTextMessage(jsonToString(message));
}

void Server::fail(QWebSocket *client, const QString &code, const QString &message)
{
    send(client, envelope("Error", {{"code", code}, {"message", message}}));
}

void Server::read(QWebSocket *client, const QString &text)
{
    QJsonObject message;
    QString error;
    if (!stringToJson(text, message, error)) {
        fail(client, "InvalidJson", error);
        return;
    }
    handler(client, message);
}

void Server::handler(QWebSocket *client, const QJsonObject &message)
{
    const QString type = message.value("type").toString();
    QString error;
    if (type == "ReadData") {
        QJsonObject data;
        if (!readData(data, error)) {
            fail(client, "ReadFailed", error);
            return;
        }
        send(client, envelope("Data", {{"data", data}}));
    } else if (type == "WriteData") {
        const auto payload = message.value("payload");
        if (!payload.isObject() || !payload.toObject().value("data").isObject()) {
            fail(client, "InvalidPayload", "payload.data must be a JSON object");
            return;
        }
        if (!writeData(payload.toObject().value("data").toObject(), error)) {
            fail(client, "WriteFailed", error);
            return;
        }
        send(client, envelope("DataWritten", {{"ok", true}}));
    } else {

        fail(client, "UnknownType", "Supported types: ReadData, WriteData");
    }
}

bool Server::writeData(const QJsonObject &data, QString &error) const
{
    QSaveFile file(m_dataFile);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }
    const QByteArray bytes = QJsonDocument(data).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        error = file.errorString();
        return false;
    }
    return true;
}

bool Server::readData(QJsonObject &data, QString &error) const
{
    QFile file(m_dataFile);
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        error = file.errorString();
        return false;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("Data file must contain a valid JSON object");
        return false;
    }
    data = document.object();
    return true;
}
