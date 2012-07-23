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
#define OOB "\xff\xff\xff\xff"

using namespace unv;

void colorize(QString &s)
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

    quint8 k = 0;

    for (quint8 i = 0; i < s.length(); ++i) {
        if (s.at(i).unicode() != '^')
            continue;

        char index = s.at(i + 1).unicode() - '0';

        if (index < 0)
            continue;

        if (index > colors.size())
            index &= colors.size() - 1;

        ++k;
        s.replace(i, 2, "<span style='color: " % colors.at(index).name() % "'>");
    }

    for ( ; k > 0; --k)
        s.append("</span>");
}

Server::Server(const QString &host, quint16 port, const QByteArray &queryMessage = "")
    : m_queryMessage(queryMessage)
{
    connect(&sock, SIGNAL(readyRead()), this, SLOT(receiveData()));
    sock.connectToHost(host, port);

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

Player GameServer::constructPlayer(Player::Team t, const QByteArray &entry)
{
    QList<QByteArray> ss = entry.left(entry.indexOf(" \"")).split(' ');

    Q_ASSERT(ss.length() == 2);

    QString s = entry.mid(entry.indexOf("\"") + 1, entry.length() - entry.indexOf("\"") - 2);
    s = Qt::escape(s);
    colorize(s);

    // the order of evaluation in argument lists is undefined, so…
    auto score = ss.takeFirst().toInt();
    auto ping  = ss.takeFirst().toInt();
    return Player(t, s, score, ping);
}

void GameServer::parseP(const QByteArray &pString, const QList<QByteArray> &pList)
{
    m_players.clear();

    for (int i = 0, j = 0; i < pString.size(); ++i) {
        int t = pString.at(i);

        if (t == '-')
            continue;
        else if ((t - '0') >= 0 && (t - '0') <= 2)
            m_players << constructPlayer(static_cast<Player::Team>(t - '0'), pList.at(j++));
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

    if (Q_LIKELY(responseType == OOB "statusResponse")) {
        info.name = kvs.value("sv_hostname", "unknown");
        info.mapname = kvs.value("mapname", "unknown");
        info.maxclients = kvs.value("sv_maxclients", 0).toInt();
        info.pure = kvs.value("pure", "1").toInt() != 0;
        info.game = kvs.value("gamename", "base");

        if (!st.isEmpty() && !(st.first() == ""))
            parseP(kvs.value("P", ""), st);

        colorize(info.name);
        emit ready();
    } else { Q_ASSERT(false); }
}

QString GameServer::formattedClientCount(const QString &fmt) const
{
    return fmt.arg(m_players.length()).arg(info.maxclients);
}

QString GameServer::formattedAddress(const QString &fmt) const
{
    return fmt.arg(sock.peerName()).arg(sock.peerPort());
}

void MasterServer::processOOB(QList<QByteArray> st) {
    if (st.length() != 1)
        Q_ASSERT(false);

    QList<QByteArray> oob = st.takeFirst().split('\\');

    if (oob.takeFirst() != OOB "getserversResponse")
        Q_ASSERT(false);

    // deleting «EOT\0\0\0»
    oob.removeLast();

    gameServers.clear();

    for (auto s : oob) {
        quint32 ip = 0;

        for (int j = 0, k = 24; j < 4; ++j, k -= 8)
            ip |= static_cast<quint8>(s.at(j)) << k;

        quint16 port = (s.at(4) << 8) + s.at(5);

        GameServer *sv = new GameServer(QHostAddress(ip).toString(), port);
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
