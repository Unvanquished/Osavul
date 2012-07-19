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

#include <QString>
#include <QTcpSocket>
#include <QWidget>
#include <QTime>
#include <QStringBuilder>
#include <QHash>
#include <QString>
#include <QSettings>
#include <QLatin1String>
#include <QTextDocument>
#include "channel.h"

namespace IrcUtil {
    QString clean(const QString &user);
    QString coloredName(const QString &peer);
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
    const QString &realName() const { return m_realName; }
    const QString &userName() const { return m_userName; }

    bool isConnected() { return sock.isOpen(); }

protected:

private:
    QTcpSocket sock;

    QSettings settings;

    QString m_nickName;
    QString m_realName;
    QString m_userName;

    QString clean(const QString &user);
    QString coloredName(const QString &peer);

    void process(QByteArray ins);
    QString htmlize(const QString &message);

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
    void setRealName(const QString &name)
    {
        m_realName = name;
        settings.setValue("realName", name);
        emit realNameChanged(name);
    }
    void setUserName(const QString &name)
    {
        m_userName = name;
        settings.setValue("userName", name);
        emit userNameChanged(name);
    }

    void join(Channel *channel);
    void join(const QString &channel);
    void part(Channel *channel);
    void part(const QString &channel);
    void send(const QString &data);

private slots:
    void receive();

signals:
    void serverCommMessage(const QString &x);

    void nickNameChanged(const QString &);
    void realNameChanged(const QString &);
    void userNameChanged(const QString &);

    void addStringToChannel(Channel *channel, const QString &string);
};

#endif // IRCCLIENT_H
