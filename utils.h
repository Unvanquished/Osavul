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

#ifndef OSAVUL_TYPES_H__
#define OSAVUL_TYPES_H__

#include <QtNetwork/QHostAddress>

namespace Settings
{
    class PreferredIPVersion
    {
    public:
        PreferredIPVersion(QAbstractSocket::NetworkLayerProtocol prefer_ = QAbstractSocket::IPv4Protocol)
            : prefer(prefer_)
            { }
        PreferredIPVersion(int prefer_)
            : prefer(QAbstractSocket::NetworkLayerProtocol(prefer_))
            { }

        bool v4() const { return prefer == QAbstractSocket::IPv4Protocol; }
        bool v6() const { return prefer == QAbstractSocket::IPv6Protocol; }
        bool any() const { return prefer == QAbstractSocket::UnknownNetworkLayerProtocol; }

        explicit operator QAbstractSocket::NetworkLayerProtocol() const { return prefer; }
        operator int() const { return int(prefer); }
        operator QVariant() { return QVariant(int(prefer)); }

        // note: returns false if any is preferred
        bool isPreferredType(const QAbstractSocket &addr)
        {
            return addr.state() >= QAbstractSocket::ConnectingState && addr.peerAddress().protocol() == prefer;
        }

    protected:
        QAbstractSocket::NetworkLayerProtocol prefer;
    };

    const PreferredIPVersion preferredIPVersion_v4 = QAbstractSocket::IPv4Protocol;
    const PreferredIPVersion preferredIPVersion_v6 = QAbstractSocket::IPv6Protocol;
    const PreferredIPVersion preferredIPVersion_any = QAbstractSocket::UnknownNetworkLayerProtocol;
}

#endif
