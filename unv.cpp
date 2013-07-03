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

#include "unv.h"

#define FFFF "\xff\xff\xff\xff"

using namespace unv;

// CAUTION: mutates the argument
void htmlColorize(QString &s)
{
    static const QList<QColor> colors = {
        QColor::fromRgbF(0.20, 0.20, 0.20), // 0 - black     00
        QColor::fromRgbF(1.00, 0.00, 0.00), // 1 - red       01
        QColor::fromRgbF(0.00, 1.00, 0.00), // 2 - green     02
        QColor::fromRgbF(1.00, 1.00, 0.00), // 3 - yellow    03
        QColor::fromRgbF(0.00, 0.00, 1.00), // 4 - blue      04
        QColor::fromRgbF(0.00, 1.00, 1.00), // 5 - cyan      05
        QColor::fromRgbF(1.00, 0.00, 1.00), // 6 - purple    06
        QColor::fromRgbF(1.00, 1.00, 1.00), // 7 - white     07
        QColor::fromRgbF(1.00, 0.50, 0.00), // 8 - orange    08
        QColor::fromRgbF(0.50, 0.50, 0.50), // 9 - md.grey   09
        QColor::fromRgbF(0.75, 0.75, 0.75), // : - lt.grey   10
        QColor::fromRgbF(0.75, 0.75, 0.75), // ; - lt.grey   11
        QColor::fromRgbF(0.00, 0.50, 0.00), // < - md.green  12
        QColor::fromRgbF(0.50, 0.50, 0.00), // = - md.yellow 13
        QColor::fromRgbF(0.00, 0.00, 0.50), // > - md.blue   14
        QColor::fromRgbF(0.50, 0.00, 0.00), // ? - md.red    15
        QColor::fromRgbF(0.50, 0.25, 0.00), // @ - md.orange 16
        QColor::fromRgbF(1.00, 0.60, 0.10), // A - lt.orange 17
        QColor::fromRgbF(0.00, 0.50, 0.50), // B - md.cyan   18
        QColor::fromRgbF(0.50, 0.00, 0.50), // C - md.purple 19
        QColor::fromRgbF(0.00, 0.50, 1.00), // D             20
        QColor::fromRgbF(0.50, 0.00, 1.00), // E             21
        QColor::fromRgbF(0.20, 0.60, 0.80), // F             22
        QColor::fromRgbF(0.80, 1.00, 0.80), // G             23
        QColor::fromRgbF(0.00, 0.40, 0.20), // H             24
        QColor::fromRgbF(1.00, 0.00, 0.20), // I             25
        QColor::fromRgbF(0.70, 0.10, 0.10), // J             26
        QColor::fromRgbF(0.60, 0.20, 0.00), // K             27
        QColor::fromRgbF(0.80, 0.60, 0.20), // L             28
        QColor::fromRgbF(0.60, 0.60, 0.20), // M             29
        QColor::fromRgbF(1.00, 1.00, 0.75), // N             30
        QColor::fromRgbF(1.00, 1.00, 0.50)  // O             31
    };

    static const QString openTag  = "<font color='%1'>";
    static const QString closeTag = "</font>";

    quint8 k = 0;

    for (quint8 i = 0; i < s.length(); ++i) {
        if (s.at(i).unicode() != '^')
            continue;

        char index = s.at(i + 1).unicode() - '0';

        if (index < 0)
            continue;

        if (index == '^' - '0') {
            s.remove(i, 1);
            continue;
        }

        if (index > colors.size())
            index &= colors.size() - 1;

        if (index == 7) {
            // ensure readability by letting the system style choose the base color
            if (!k)
                s.remove(i, 2);
            else {
                // cancel out all (previous) colors
                s.remove(2 * k);
                while (k > 0)
                    s.insert(i, closeTag), --k;
            }

            continue;
        }

        s.replace(i, 2, openTag.arg(colors.at(index).name()));
        ++k, ++i;
    }

    while (k > 0)
        s.append(closeTag), --k;
}

Server::Server(const QHostAddress &host, quint16 port, const QByteArray &queryMessage = "")
    : m_queryMessage(queryMessage)
{
    connect(&sock, SIGNAL(readyRead()), this, SLOT(receiveData()));
    sock.connectToHost(host, port);

    m_ipv6 = host.protocol() == QAbstractSocket::IPv6Protocol;

    if (!sock.waitForConnected())
        return;
}

Server::Server(const QString &host, quint16 port, const QByteArray &queryMessage = "")
    : m_queryMessage(queryMessage)
{
    connect(&sock, SIGNAL(readyRead()), this, SLOT(receiveData()));
    sock.connectToHost(host, port);

    m_ipv6 = sock.peerAddress().protocol() == QAbstractSocket::IPv6Protocol;

    if (!sock.waitForConnected())
        return;
}

void Server::receiveData()
{
    if (m_ping == 0 && pingTimer.isValid()) {
        m_ping = pingTimer.elapsed();
        pingTimer.invalidate();
    }

    // 1400 is the maximum #bytes per packet, defined by masterserver code.
    // replies of the more popular servers of Tremulous are still much smaller.
    // stay on alert if it ever changes, though.

    while (sock.hasPendingDatagrams()) {
        QByteArray tmp = sock.read(1400);
        m_infoString = QString(tmp);
        processOOB(tmp.split('\n'));
    }
}

void Server::query()
{
    sock.write(m_queryMessage);

    m_ping = 0;
    pingTimer.start();
}

Server::~Server()
{
    sock.close();
}

Player GameServer::constructPlayer(Player::Team t, const QByteArray &entry, bool isBot)
{
    QList<QByteArray> ss = entry.left(entry.indexOf(" \"")).split(' ');

    Q_ASSERT(ss.length() == 2);

    QString s = entry.mid(entry.indexOf("\"") + 1, entry.length() - entry.indexOf("\"") - 2);
    s = Qt::escape(s);
    htmlColorize(s);

    // the order of evaluation in argument lists is undefined, so…
    auto score = ss.takeFirst().toInt();
    auto ping  = ss.takeFirst().toInt();
    return Player(t, s, score, ping, isBot);
}

void GameServer::parsePB(const QByteArray &pString, const QByteArray &bString, const QList<QByteArray> &pList)
{
    m_players.clear();
    bool bDefined = bString.size() != 0;

    for (int i = 0, j = 0; i < pString.size(); ++i) {
        int t = pString.at(i);

        if (t == '-')
            continue;
        else if ((t - '0') >= 0 && (t - '0') <= 2)
            m_players << constructPlayer(static_cast<Player::Team>(t - '0'), pList.at(j++), bDefined && bString.at(i) == 'b');
        else
            Q_ASSERT(false);
    }
}

void GameServer::processOOB(QList<QByteArray> st)
{
    QByteArray responseType = st.takeFirst();
    QList<QByteArray> kvList = st.takeFirst().split('\\');
    kvList.removeAll(""); // only QString can split taking a SkipEmptyParts enum member

    Q_ASSERT(!(kvList.length() % 2));

    QHash<QByteArray, QByteArray> kvs;

    for (int i = 0; i < kvList.length(); i += 2)
        kvs.insert(kvList.at(i), kvList.at(i + 1));

    if (Q_LIKELY(responseType == FFFF "statusResponse")) {
        info.name = kvs.value("sv_hostname", "unknown");
        info.mapname = kvs.value("mapname", "unknown");
        info.statsURL = kvs.value("sv_statsURL", nullptr);
        info.maxclients = kvs.value("sv_maxclients", 0).toInt();
        info.pure = kvs.value("pure", "1").toInt() != 0;
        info.game = kvs.value("gamename", "base");

        if (!st.isEmpty() && !(st.first() == ""))
            parsePB(kvs.value("P", ""), kvs.value("B", ""), st);

        m_lastUpdateTime = QDateTime::currentDateTime();

        htmlColorize(info.name);
        emit ready();
    } else { Q_ASSERT(false); }
}

int GameServer::botCount() const
{
    int i, lim = m_players.length(), bots = 0;

    for (i = 0; i < lim; ++i)
        if (m_players[i].isBot())
            ++bots;

    return bots;
}

QString GameServer::formattedClientCount(const QString &fmt) const
{
    int bots = botCount();
    QString arg1;

    if (bots)
    {
        static const QString xfmt = "%1+%2";
        arg1 = xfmt.arg(m_players.length() - bots).arg(bots);
        return fmt.arg(arg1).arg(info.maxclients);
    }

    return fmt.arg(m_players.length()).arg(info.maxclients);
}

QString GameServer::formattedAddress(const QString &fmt) const
{
    return fmt.arg(sock.peerName()).arg(sock.peerPort());
}

void MasterServer::processOOB(QList<QByteArray> st) {
    if (st.length() != 1)
        Q_ASSERT(false);

    QByteArray oob = st.takeFirst();

    int index;
    bool extended = false;
    const int length = oob.length();

    if (length > 22 && oob.mid(0, 22) == FFFF "getserversResponse" && oob.at(22) == 0)
    {
        index = 23;
        gameServers.clear();
    }
    else if (length > 25 && oob.mid(0, 25) == FFFF "getserversExtResponse\0" && oob.at(25) == 0)
    {
        index = 26;
        extended = true;
        if ( oob.mid(26).toInt() == 1)
            gameServers.clear();
    }
    else
        Q_ASSERT(false);

    while (index < length)
    {
        char c = oob.at(index);
        if (c == '\\' || (extended && c == '/'))
            break;
        ++index;
    }

    while (index < length)
    {
        union {
            quint32 v4;
            quint8  v6[16];
        } ip;
        quint16 port;
        bool isv6;

        switch (oob.at(index++))
        {
            case '\\':
                if (index + 6 > length)
                    return;
                ip.v4 = 0;

                for (int j = 0; j < 4; ++j)
                    ip.v4 |= static_cast<quint8>(oob.at(index++)) << (24 - j * 8);
                isv6 = false;
                break;

            case '/':
                if (!extended)
                    return; // rest is invalid unless response is extended-mode
                if (index + 18 > length)
                    return;

                memcpy(ip.v6, oob.data() + index, 16);
                index += 16;
                isv6 = true;
                break;

            default: // invalid
                return;
        }


        port = (oob.at(index) << 8) + oob.at(index + 1);
        index += 2;

        GameServer *sv = isv6
                       ? new GameServer(QHostAddress(ip.v6), port)
                       : new GameServer(QHostAddress(ip.v4), port);
        gameServers << sv;
        connect(sv, SIGNAL(ready()), this, SLOT(onGameSvReady()));
        sv->query();
    }
}

void MasterServer::onGameSvReady() {
    GameServer *sv = qobject_cast<GameServer *> (sender());
    if (!sv) { Q_ASSERT(false); }

    emit serverQueried(sv);
}

#undef FFFF
