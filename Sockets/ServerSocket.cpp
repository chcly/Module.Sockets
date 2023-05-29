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
#include "Sockets/ServerSocket.h"
#include <thread>
#include "SocketStream.h"
#include "Sockets/Connection.h"
#include "Thread/Runner.h"
#include "Utils/Exception.h"

namespace Rt2::Sockets
{
    class ServerThread final : public Thread::Runner
    {
    private:
        bool                _status{true};
        const ServerSocket* _socket{nullptr};

    private:
        void update() override
        {
            while (isRunningUnlocked())
            {
                Net::Connection client;
                if (const Net::Socket sock = Net::accept(_socket->_server, client);
                    sock != Net::InvalidSocket)
                {
                    _socket->connected(sock);
                    Net::close(sock);
                }
                else
                {
                    // non-blocking, so exit signal handlers can
                    // process an exit gracefully.
                    Thread::Thread::yield();
                }
            }
        }

    public:
        explicit ServerThread(const ServerSocket* parent) :
            _socket(parent)
        {
        }
    };

    ServerSocket::ServerSocket(const String&  ipv4,
                               const uint16_t port,
                               const uint16_t backlog)
    {
        Net::ensureInitialized();
        open(ipv4, port, backlog);
    }

    ServerSocket::~ServerSocket()
    {
        stop();

        if (_server)
        {
            Net::close(_server);
            _server = Net::InvalidSocket;
        }
    }

    void ServerSocket::start()
    {
        if (!_main)
        {
            _main = new ServerThread(this);
            _main->start();
        }
    }

    void ServerSocket::stop()
    {
        if (_main)
        {
            _main->stop();
            delete _main;
            _main = nullptr;
        }
        _status = -5;
    }

    void ServerSocket::connect(const ConnectionAccepted& onAccept)
    {
        _accepted = onAccept;
    }

    void ServerSocket::connected(const Net::Socket& socket) const
    {
        if (_accepted)
            _accepted(socket);
    }

    void ServerSocket::open(const String& ipv4, uint16_t port, const uint16_t backlog)
    {
        using namespace Net;
        try
        {
            _server = create(AddrINet,
                             Stream,
                             ProtoIpTcp,
                             false);
            if (_server == InvalidSocket)
                throw Exception("failed to create socket");

            constexpr int opt = 1;
            if (setOption(_server,
                          SocketLevel,
                          ReuseAddress,
                          &opt,
                          sizeof(int)) != Ok)
            {
                throw Exception("Failed to set socket option");
            }

            SocketInputAddress host;
            constructInputAddress(host, AddrINet, port, ipv4);

            if (bind(_server, host) != Ok)
                throw Exception("Failed to bind server socket to ", ipv4, ':', port);

            if (listen(_server, backlog) != Ok)
                throw Exception("Failed to listen on the server socket");

            _status = 0;
        }
        catch (Exception& ex)
        {
            Console::writeError(ex.what());
            _status = -2;
        }
    }

}  // namespace Rt2::Sockets
