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

namespace IrcUtil {
    QString clean(const QString &user)
    {
        return user.split('!').first();
    }

    QString coloredName(const QString &peer)
    {
        static const QString templ = "<span style='color: %1'>%2</span>";
        static const int colorsTotal = QColor::colorNames().length();
        static QHash<QString, QColor> peerColors;

        if (!peerColors.contains(peer))
            peerColors.insert(peer, QColor(QColor::colorNames().at(qrand() % colorsTotal)));

        return templ.arg(peerColors.value(peer).name(), clean(peer));
    }
}

IrcClient::IrcClient(QObject *parent) : QObject(parent)
{
    connect(&sock, SIGNAL(readyRead()), this, SLOT(receive()));
    settings.beginGroup("irc");
}

void IrcClient::connectToHost(const QString &host, quint16 port)
{
    sock.connectToHost(host, port);

    if (!sock.waitForConnected())
        return;
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
    qDebug() << "<<" << data;
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

    send("JOIN " % name);
    channels.insert(name, channel);
}

void IrcClient::join(const QString &channel)
{
    send("JOIN " % channel);
}

void IrcClient::part(Channel *channel)
{
    const QString &name = channel->channel();

    if (!channels.contains(name))
        return;

    send("PART " % name % " :Osavul parted");
    channels.remove(name);
}

void IrcClient::part(const QString &channel)
{
    send("PART " % channel);
}

QString htmlized(const QString& message)
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

void IrcClient::receive()
{
    static QByteArray buf;

    while (!sock.atEnd()) {
        if (!sock.canReadLine()) {
            buf += sock.readAll();
            continue;
        }

        QByteArray rawLine;

        if (buf.isEmpty()) {
            rawLine = sock.readLine();
        } else {
            rawLine = buf + sock.readLine();
            buf.clear();
        }

        if (rawLine.startsWith("PING")) {
            sock.write("PONG " % rawLine.split(' ').last());
            continue;
        }

        // no CRLF, please
        QByteArray in = rawLine.left(rawLine.length() - 2);

        qDebug() << in;

        // no prefix ':' either
        if (in.startsWith(':'))
            in.remove(0, 1);

        int last = in.indexOf(" :");

        QList<QByteArray> params =
                last < 0 ? in.split(' ') : in.mid(0, last).split(' ') << in.mid(last + 2);

        if (params.length() < 3)
            continue;

        QString peer   = QString::fromUtf8(params.takeFirst());
        QString cmd    = QString::fromUtf8(params.takeFirst());
        QString target = QString::fromUtf8(params.takeFirst());
        QString what   = QString::fromUtf8(params.isEmpty() ? "" : params.takeFirst());

        QString out;

        if (cmd == "353") {
            Channel *chan = channels.value(params.takeFirst());

            for (const QByteArray &user : params.takeFirst().split(' '))
                chan->addUser(user);
        }

        if (!channels.contains(target)) {
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
            addStringToChannel(channels.value(target), out.arg(IrcUtil::coloredName(peer), target));
            continue;
        }

        if (cmd == "PRIVMSG")
            out = "(%1) %2";
        else if (cmd == "NOTICE")
            out = "(NOTICE from %1) %2";

        if (!out.isEmpty())
            addStringToChannel(channels.value(target), out.arg(IrcUtil::coloredName(peer), what));
        else
            serverCommMessage(in);
    }
}

void IrcClient::die() {
    send("QUIT :Osavul closed");
    sock.disconnectFromHost();
    sock.close();
}
