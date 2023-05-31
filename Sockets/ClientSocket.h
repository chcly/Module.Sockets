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
#include "Utils/Definitions.h"

namespace Rt2::Sockets
{
    class ClientSocket
    {
    private:
        Net::Socket _client{Net::InvalidSocket};

    public:
        ClientSocket(const String& ipv4, uint16_t port);
        ClientSocket();
        ~ClientSocket();

        void write(const String& msg) const;

        void write(IStream& msg) const;

        bool isOpen() const;

        void open(const String& ipv4, uint16_t port);

        const Net::Socket& socket() const;

        void close();
    };

    inline bool ClientSocket::isOpen() const
    {
        return _client != Net::InvalidSocket;
    }

    inline const Net::Socket& ClientSocket::socket() const
    {
        return _client;
    }

}  // namespace Rt2::Sockets
