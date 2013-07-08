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

#ifndef UNV_H
#define UNV_H

#include <QtNetwork/QUdpSocket>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringBuilder>
#include <QtCore/QDateTime>
#include <QtCore/QSettings>
#include <QtGui/QColor>
#include <QtGui/QTextDocument> // for Qt::escape()

#define FFFF "\xff\xff\xff\xff"

namespace unv {
    class Server : public QObject
    {
        Q_OBJECT

    public:
        Server(const QHostAddress &host, quint16 port, const QByteArray &queryMessage);
        Server(const QString &host, quint16 port, const QByteArray &queryMessage);

        void addAddress(const QHostAddress &host, quint16 port);

        QString host(int index = 0) const { return validIndex(index) ? sock[index].sock.peerName() : ""; }
        quint16 port(int index = 0) const { return validIndex(index) ? sock[index].sock.peerPort() : 0; }
        quint16 ping(int index = 0) const { return validIndex(index) ? sock[index].ping : 0; }
        bool    ipv4() const;
        bool    ipv6() const;
        bool    has(const QHostAddress &host, quint16 port) const;

        const QString &infoString() const { return m_infoString; }

    protected:
        ~Server();
        virtual void processOOB(QByteArray st) = 0;
        const QByteArray m_queryMessage;

        struct {
            QUdpSocket sock;
            QElapsedTimer pingTimer;
            int ping;
        } sock[2];

        bool validIndex(int index) const { return index >= 0 && index < 2; }

        const QSettings settings;

    private:
        QString m_infoString;

    public slots:
        void query();
    protected slots:
        void receiveData();
    };

    class Player
    {
    public:
        enum Team {Spectators = 0, Aliens = 1, Humans = 2};
        Player(Team t = Spectators, const QString &name = "unknown",
               qint16 score = 0, quint16 ping = 0, bool isBot = false)
            : m_team(t), m_name(name), m_plainName(QString(name).replace(QRegExp("<[^>]*>"), ""))
            , m_score(score), m_ping(ping), m_bot(isBot) { }

        bool operator <(const Player &other) const { return m_score < other.score(); }
        bool operator >(const Player &other) const { return m_score > other.score(); }

        Team team() const { return m_team; }
        const QString &name() const { return m_name; }
        const QString &plainName() const { return m_plainName; }
        qint16  score() const { return m_score; }
        quint16 ping() const  { return m_ping; }
        bool    isBot() const { return m_bot; }

    private:
        Team m_team;
        QString m_name;
        QString m_plainName;
        qint16  m_score;
        quint16 m_ping;
        bool m_bot;
    };

    class GameServer : public Server
    {
        Q_OBJECT

    public:
        GameServer(const QHostAddress &host, quint16 port)
            : Server(host, port, FFFF "getstatus"), info() { }
        GameServer(const QString &host, quint16 port)
            : Server(host, port, FFFF "getstatus"), info() { }
        ~GameServer() { }

        QString game() const { return info.game; }
        QString map() const  { return info.mapname; }
        QString name() const { return info.name; }
        QString statsURL() const { return info.statsURL; }

        const QList<Player> &players() const { return m_players; }
        const QDateTime &lastUpdateTime() const { return m_lastUpdateTime; }
        bool isPure() const { return info.pure; }

        int botCount() const;

        QString formattedAddress(bool usePreferred, const QString &fmt = "%1:%2") const;
        QString formattedAddress(int which = 0, const QString &fmt = "%1:%2") const;
        QString formattedClientCount(const QString &fmt = "[%1/%2]") const;

    private:
        Q_DISABLE_COPY(GameServer)
        Player constructPlayer(Player::Team t, const QByteArray &entry, bool isBot = false);
        void parsePB(const QByteArray &pString, const QByteArray &bString, const QList<QByteArray> &p2);

        struct {
            QString game;
            QString mapname;
            QString name;
            QString statsURL;
            quint8 clients;
            quint8 maxclients;
            bool pure;
        } info;

        QList<Player> m_players;
        QDateTime m_lastUpdateTime;

        void processOOB(QByteArray st);

    signals:
        void ready();
    };

    class LinkedAddressPair : public QObject
    {
        Q_OBJECT

    public:
        LinkedAddressPair(const QHostAddress &ip4, quint16 portno4, const QHostAddress &ip6, quint16 portno6)
            : ipv4(ip4), ipv6(ip6), port4(portno4), port6(portno6)
        { }
        ~LinkedAddressPair() { }

        bool contains(const QHostAddress &ip, quint16 port) const
        {
            return (ipv4 == ip && port4 == port ) || (ipv6 == ip && port6 == port);
        }

        const QHostAddress &getOtherAddress(const QHostAddress &ip) const
        {
            return ipv4 == ip ? ipv6 : ipv4;
        }

        quint16 getOtherPort(const QHostAddress &ip) const
        {
            return ipv4 == ip ? port6 : port4;
        }

    private:
        QHostAddress ipv4, ipv6;
        quint16 port4, port6;
    };

    class MasterServer : public Server
    {
        Q_OBJECT

    public:
        MasterServer(const QString &host, quint16 port, quint16 protocol)
            : Server(host, port, FFFF "getserversExt UNVANQUISHED " % QByteArray::number(protocol) % " dual full empty")
        { }
        ~MasterServer() { }

        const QList<GameServer *> &servers() { return gameServers; }

    private:
        Q_DISABLE_COPY(MasterServer)
        QList<GameServer *> gameServers;
        QList<LinkedAddressPair *> linkedAddressPairs;
        void processOOB(QByteArray st);
        QList<GameServer *> *parseServersOOB(const QByteArray &oob, int index, int length, bool extended);

    signals:
        void serverQueried(unv::GameServer *sv);

    public slots:
        void onGameSvReady();
    };

    struct FavoriteEntry {
        QString name;
        QString host;
        quint16 port;

        bool operator ==(const FavoriteEntry &other) const
        {
            return (name == other.name && host == other.host && port == other.port);
        }
    };
}

Q_DECLARE_METATYPE(unv::GameServer *)
Q_DECLARE_METATYPE(unv::FavoriteEntry *)

#undef FFFF
#endif // UNV_H
