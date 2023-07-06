/*
-------------------------------------------------------------------------------
    Copyright (c) Charles Carley.

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#pragma once
#include "Sockets/PlatformSocket.h"

namespace Rt2::Sockets
{
    class Socket
    {
    protected:
        PlatformSocket _sock{InvalidSocket};
        AddressFamily  _family{AddressFamilyUnknown};
        SocketType     _type{SocketUnknown};
        Protocol       _protocol{ProtocolUnknown};

    public:
        Socket();

        virtual ~Socket();

        bool isValid() const;

        void setBlocking(bool val) const;

        void setKeepAlive(bool val) const;

        bool keepAlive() const;

        void setReuseAddress(bool val) const;

        bool reuseAddress() const;

        void setBroadcast(bool val) const;

        bool isBroadcasting() const;

        void setDebug(bool val) const;

        bool isDebug() const;

        void setRoute(bool val) const;

        bool isRouting() const;

        void setMaxSendBuffer(int max) const;

        int maxSendBuffer() const;

        void setMaxReceiveBuffer(int max) const;

        int maxReceiveBuffer() const;

        void setSendTimeout(int ms) const;

        int sendTimeout() const;

        void setReceiveTimeout(int ms) const;

        int receiveTimeout() const;

        const PlatformSocket& socket() const;

        const AddressFamily& family() const;

        const Protocol& protocol() const;

        const SocketType& type() const;

        void setFamily(const AddressFamily& family);

        void setProtocol(const Protocol& protocol);

        void setType(const SocketType& type);

        void close();

        void create();
    };

    inline bool Socket::isValid() const
    {
        return _sock != InvalidSocket;
    }

    inline const AddressFamily& Socket::family() const
    {
        return _family;
    }

    inline const Protocol& Socket::protocol() const
    {
        return _protocol;
    }

    inline const SocketType& Socket::type() const
    {
        return _type;
    }

    inline void Socket::setFamily(const AddressFamily& family)
    {
        _family = family;
    }

    inline void Socket::setProtocol(const Protocol& protocol)
    {
        _protocol = protocol;
    }

    inline void Socket::setType(const SocketType& type)
    {
        _type = type;
    }

    inline const PlatformSocket& Socket::socket() const
    {
        return _sock;
    }

}  // namespace Rt2::Sockets
