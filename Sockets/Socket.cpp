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
#include "Sockets/Socket.h"

namespace Rt2::Sockets
{
    Socket::Socket()
    {
        Net::ensureInitialized();
    }

    Socket::~Socket()
    {
        close();
    }

    void Socket::setBlocking(const bool val) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, Blocking, val);
    }

    void Socket::setKeepAlive(const bool val) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, KeepAlive, val);
    }

    bool Socket::keepAlive() const
    {
        RT_GUARD_RET(isValid(), false)
        return Net::optionBool(_sock, KeepAlive);
    }

    void Socket::setReuseAddress(const bool val) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, ReuseAddress, val);
    }

    bool Socket::reuseAddress() const
    {
        RT_GUARD_RET(isValid(), false)
        return Net::optionBool(_sock, ReuseAddress);
    }

    void Socket::setBroadcast(const bool val) const
    {
        RT_GUARD_VOID(isValid() && _type == SocketDatagram)
        Net::setOption(_sock, Broadcast, val);
    }

    bool Socket::isBroadcasting() const
    {
        RT_GUARD_RET(isValid() && _type == SocketDatagram, false)
        return Net::optionBool(_sock, Broadcast);
    }

    void Socket::setDebug(const bool val) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, Debug, val);
    }

    bool Socket::isDebug() const
    {
        RT_GUARD_RET(isValid(), false)
        return Net::optionBool(_sock, Debug);
    }

    void Socket::setRoute(const bool val) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, DoNotRoute, !val);
    }

    bool Socket::isRouting() const
    {
        RT_GUARD_RET(isValid(), false)
        return Net::optionBool(_sock, DoNotRoute) == false;
    }

    void Socket::setMaxSendBuffer(const int max) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, SendBufferSize, max);
    }

    int Socket::maxSendBuffer() const
    {
        RT_GUARD_RET(isValid(), 0)
        return Net::optionInt(_sock, SendBufferSize);
    }

    void Socket::setMaxReceiveBuffer(const int max) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, ReceiveBufferSize, max);
    }

    int Socket::maxReceiveBuffer() const
    {
        RT_GUARD_RET(isValid(), 0)
        return Net::optionInt(_sock, ReceiveBufferSize);
    }

    void Socket::setSendTimeout(const int ms) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, SendTimeout, ms);
    }

    int Socket::sendTimeout() const
    {
        RT_GUARD_RET(isValid(), 0)
        return Net::optionInt(_sock, SendTimeout);
    }

    int Socket::receiveTimeout() const
    {
        RT_GUARD_RET(isValid(), 0)
        return Net::optionInt(_sock, ReceiveTimeout);
    }

    void Socket::setReceiveTimeout(const int ms) const
    {
        RT_GUARD_VOID(isValid())
        Net::setOption(_sock, ReceiveTimeout, ms);
    }

    void Socket::close()
    {
        if (_sock != InvalidSocket)
        {
            Net::close(_sock);
            _sock = InvalidSocket;
        }
    }

    void Socket::create()
    {
        close();

        // defaults
        if (_family == AddressFamilyUnknown)
            _family = AddressFamilyINet;
        if (_type == SocketUnknown)
            _type = SocketStream;

        // allow PF_UNSPEC
        _sock = Net::create(_family, _type, _protocol);
    }
}  // namespace Rt2::Sockets
