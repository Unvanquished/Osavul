/*
* This file is part of Osavul.
*
* Osavul is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Osavul is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Osavul.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IRCCLIENT_H
#define IRCCLIENT_H

#include "channel.h"
#include <QtNetwork/QTcpSocket>
#include <QtCore/QStringBuilder>
#include <QtCore/QSettings>
#include <QtGui/QTextDocument> // for Qt::escape()

namespace IrcUtil {
    QString clean(const QString &user);
    QString coloredName(const QString &peer);
    QString htmlized(const QString &message);
}

class IrcClient : public QObject
{
    Q_OBJECT

public:
    explicit IrcClient(QObject *parent = nullptr);
    ~IrcClient();

    QString host() const { return sock.peerName(); }
    quint16 port() const { return sock.peerPort(); }

    void connectToHost(const QString &host, quint16 port);
    const QString &nickName() const { return m_nickName; }
    bool isConnected() { return sock.isOpen(); }

protected:

private:
    QTcpSocket sock;
    QSettings settings;
    QString m_nickName;

    QHash<QString, Channel *> channels;

public slots:
    void init(const QString &nickName);
    void die();
    void say(Channel *channel, const QString &data);

    void setNickName(const QString &name)
    {
        m_nickName = name;
        this->send("NICK " % name);
        emit nickNameChanged(name);
    }

    void join(Channel *channel);
    void part(Channel *channel);
    void send(const QString &data);

private slots:
    void receive();

signals:
    void serverCommMessage(const QString &);
    void nickNameChanged(const QString &);
    void addStringToChannel(Channel *, const QString &);
    void highlight(Channel *);
};

#endif // IRCCLIENT_H
