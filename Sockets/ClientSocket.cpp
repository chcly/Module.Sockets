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
#include "Sockets/ClientSocket.h"
#include "Sockets/Connection.h"
#include "Utils/Exception.h"
#include "Utils/LogFile.h"


namespace Rt2::Sockets
{
    ClientSocket::ClientSocket(const String&  ipv4,
                               const uint16_t port)
    {
        Net::ensureInitialized();
        open(ipv4, port);
    }

    ClientSocket::ClientSocket()
    {
        Net::ensureInitialized();
    }

    ClientSocket::~ClientSocket()
    {
        close();
    }

    void ClientSocket::write(const String& msg) const
    {
        if (isOpen())
            Net::writeBuffer(_client, msg.c_str(), msg.size());
    }

    void ClientSocket::write(IStream& msg) const
    {
        OutputStringStream oss;
        while (!msg.eof())
            oss.put((char)msg.get());
        write(oss.str());
    }

    void ClientSocket::open(const String& ipv4, uint16_t port)
    {
        try
        {
            if (isOpen()) close();

            _client = create(Net::AddrINet, Net::Stream, Net::ProtoUnspecified, true);
            if (_client == Net::InvalidSocket)
                throw Exception("failed to create socket");

            if (Net::connect(_client, ipv4, port) != Net::Ok)
                throw Exception("failed to connect to ", ipv4, ':', port);
        }
        catch (Exception& ex)
        {
            Log::println(ex.what());
            close();
        }
    }

    void ClientSocket::close()
    {
        if (isOpen())
        {
            Net::close(_client);
            _client = Net::InvalidSocket;
        }
    }

}  // namespace Rt2::Sockets
