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

#include "ircclient.h"

#define RED(x) "<span style='color: red'>" x "</span>"
#define IMPORTANT RED(" ! ")

namespace IrcUtil {
    QString &clean(const QString &user)
    {
        return user.split('!').first();
    }

    QString colorize(const QString &color, QString &text)
    {
        static const QString open  = "<span style='color: %1'>";
        static const QString close = "</span>";

        return text.prepend(open.arg(color)).append(close);
    }

    QString coloredName(const QString &peer)
    {
        static const int colorsTotal = QColor::colorNames().length();
        static QHash<QString, QColor> peerColors;

        if (!peerColors.contains(peer))
            peerColors.insert(peer, QColor(QColor::colorNames().at(qrand() % colorsTotal)));

        return colorize(peerColors.value(peer).name(), clean(peer));
    }

    QString htmlized(const QString &message)
    {
        QString out = Qt::escape(message);

        enum {
            Zero      = 0x00,
            Bold      = 0x01,
            Color     = 0x02,
            Italic    = 0x04,
            Underline = 0x08
        };

        int parsingState = 0;
        const char * const end = "</span>";
        QByteArray tmp;

        for (int i = 0; i < out.length(); ++i) {
            QChar c = out.at(i);
            if (c.isPrint())
                continue;

            switch (c.unicode()) {
                case '\x02': // bold
                case '\x16': // inverse
                    tmp = (parsingState & Bold) ? end : "<span style='font-weight: bold'>";
                    parsingState ^= Bold;
                    break;
                case '\x03': // color
                    tmp = (parsingState & Color) ? end : "<span style='color: %1'>";
                    parsingState ^= Color;
                    break;
                case '\x1d': // italic
                    tmp = (parsingState & Italic) ? end : "<span style='font-style: italic'>";
                    parsingState ^= Italic;
                    break;
                case '\x15': // underline
                case '\x1f': // ditto
                    tmp = (parsingState & Underline) ? end : "<span style='text-decoration: underline'>";
                    parsingState ^= Underline;
                    break;
                case '\x0f':
                    parsingState = Zero;
                    break;
                default:
                    break;
            }

            out.replace(i, 1, tmp);
            i += tmp.size();
        }

        return out;
    }
}

IrcClient::IrcClient(QObject *parent) : QObject(parent)
{
    connect(&sock, SIGNAL(readyRead()), this, SLOT(receive()));
    settings.beginGroup("irc");
}

void IrcClient::connectToHost(const QString &host, quint16 port)
{
    serverCommMessage(IMPORTANT "Connecting...");
    sock.connectToHost(host, port);

    if (!sock.waitForConnected()) {
        serverCommMessage("Connection failed!");
        sock.disconnectFromHost();
        sock.close();
    }
}

IrcClient::~IrcClient()
{
    sock.disconnectFromHost();
}

void IrcClient::init(const QString &nickName)
{
    send("USER osavul 8 * :Osavul, the Unvanquished server browser");
    this->setNickName(nickName);
}

void IrcClient::send(const QString &data)
{
    sock.write(QString(data % "\r\n").toUtf8());
}

void IrcClient::say(Channel *channel, const QString &data)
{
    static const QString templ1 = "(%1) %2";
    static const QString templ2 = "PRIVMSG %1 :%2";
    send(templ2.arg(channel->channel(), data));
    emit addStringToChannel(channel, templ1.arg(IrcUtil::coloredName(m_nickName), data));
}

void IrcClient::join(Channel *channel)
{
    const QString &name = channel->channel(); // yo dawg we heard you like channels

    if (channels.contains(name))
        return;

    if (name.startsWith('#'))
        send("JOIN " % name);

    channels.insert(name, channel);
}

void IrcClient::part(Channel *channel)
{
    const QString &name = channel->channel();

    if (!channels.contains(name))
        return;

    if (name.startsWith('#'))
        send("PART " % name % " :Osavul parted");

    channels.remove(name);
}

void IrcClient::receive()
{
    static QByteArray buf;

    while (!sock.atEnd()) {
        if (!sock.canReadLine()) {
            buf += sock.readAll();
            continue;
        }

        QByteArray rawLine;

        if (buf.isEmpty())
            rawLine = sock.readLine();
        else
            rawLine = buf + sock.readLine(), buf.clear();

        if (rawLine.startsWith("PING")) {
            sock.write("PONG " % rawLine.split(' ').last());
            continue;
        }

        // no CRLF, please
        QString in = QString::fromUtf8(rawLine.left(rawLine.length() - 2));

        // no prefix ':' either
        if (in.startsWith(':'))
            in.remove(0, 1);

                qDebug() << in;

        int last = in.indexOf(" :");

        QStringList params =
                last < 0 ? in.split(' ') : in.mid(0, last).split(' ') << in.mid(last + 2);

        if (params.length() < 3)
            continue;

        QString peer   = params.takeFirst();
        QString cmd    = params.takeFirst();
        QString target = params.takeFirst();
        QString what   = params.isEmpty() ? "" : params.takeFirst();

        QString out;

        if (cmd == "353") {
            Channel *chan = channels.value(params.takeFirst());

            for (const QString &user : params.takeFirst().split(' '))
                chan->addUser(user);
        }

        if (!channels.contains(target) && target != m_nickName) {
            serverCommMessage(in);
            continue;
        }

        if (cmd == "JOIN")
            out = tr("%1 has joined %2");
        else if (cmd == "PART")
            out = tr("%1 has left %2");
        else if (cmd == "NICK")
            out = tr("%1 is now known as %2");
        else if (cmd == "QUIT")
            out = tr("%1 has quit [%2]");

        if (!out.isEmpty()) {
            addStringToChannel(channels.value(target),
                               out.arg(IrcUtil::coloredName(peer), target));
            continue;
        }

        if (cmd == "PRIVMSG")
            out = "(%1) %2";
        else if (cmd == "NOTICE")
            out = "(NOTICE from %1) %2";

        if (!out.isEmpty()) {
            if (what.at(0) == '\001' && what.at(what.length() - 1) == '\001') {
                // whoa, CTCP!
                QStringRef ref = what.midRef(1, what.length() - 2);

                if (ref == "VERSION")
                    // WHAT TODO, qApp is inaccessible from non-GUI threads
                    send(QString("NOTICE %1 :%2").arg(IrcUtil::clean(peer)).arg("Osavul!"));

                continue;
            }

            Channel *chan = channels.value(target == m_nickName ? IrcUtil::clean(peer) : target);

            if (chan == nullptr) {
                send(QString("PRIVMSG %1 :%2").arg(IrcUtil::clean(peer)).arg("hi"));
                continue;
            }

            QString s = out.arg(IrcUtil::coloredName(peer), what);

            if (what.contains(m_nickName))
                highlight(chan), addStringToChannel(chan, IrcUtil::colorize("red", s));
            else
                addStringToChannel(chan, s);
        } else
            serverCommMessage(in);
    }
}

void IrcClient::die() {
    send("QUIT :Osavul closed");
    serverCommMessage(IMPORTANT "Disconnected.");
    sock.disconnectFromHost();
    sock.close();
}
