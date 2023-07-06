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
#include <csignal>
#include "Sockets/Connection.h"
#include "Thread/Runner.h"
#include "Thread/SharedValue.h"
#include "Utils/Exception.h"

namespace Rt2::Sockets
{
    class ServerThread final : public Thread::Runner
    {
    private:
        const ServerSocket* _socket{nullptr};

    private:
        void update() override
        {
            while (isRunningUnlocked())
            {
                RT_GUARD_CHECK_VOID(_socket && _socket->isValid())

                Connection client;
                if (const PlatformSocket sock = Net::accept(_socket->_sock, client);
                    sock != InvalidSocket)
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
        open(ipv4, port, backlog);
    }

    ServerSocket::~ServerSocket()
    {
        destroy();
        close();
    }

    void ServerSocket::open(const String& ipv4, uint16_t port, const uint16_t backlog)
    {
        try
        {
            setFamily(AddressFamilyINet);
            setType(SocketStream);
            setProtocol(ProtocolIpTcp);
            create();
            if (!isValid()) throw Exception("failed to create socket");

            setReuseAddress(true);
            setBlocking(false);

            setMaxReceiveBuffer(Default::IoBufferSize);
            setMaxSendBuffer(Default::IoBufferSize);
            setSendTimeout(Default::SocketTimeOut);
            setReceiveTimeout(Default::SocketTimeOut);

            SocketInputAddress host;
            Net::Utils::constructInputAddress(host, AddressFamilyINet, port, ipv4);

            if (Net::bind(_sock, host) != OkStatus)
                throw Exception("Failed to bind server socket to ", ipv4, ':', port);

            if (Net::listen(_sock, backlog) != OkStatus)
                throw Exception("Failed to listen on the server socket");

            start();
        }
        catch (Exception& ex)
        {
            Console::println(ex.what());
            close();
        }
    }

    void ServerSocket::start()
    {
        if (!_main)
        {
            _main = new ServerThread(this);
            _main->start();
            _running = true;
        }
    }

    void ServerSocket::destroy()
    {
        if (_main)
            _main->stop();
        delete _main;
        _main = nullptr;
    }

    void ServerSocket::run()
    {
        if (!_main)
            start();

        if (_main)
        {
            while (_running) Thread::Thread::yield();
            destroy();
        }
    }

    void ServerSocket::stop()
    {
        auto lock = Thread::ScopeLock(&_mutex);
        _running = false;
    }

    void ServerSocket::connect(const ConnectionAccepted& onAccept)
    {
        _accepted = onAccept;
    }

    void ServerSocket::signal()
    {
        //(void)std::raise(SIGINT);
    }

    void ServerSocket::connected(const PlatformSocket& socket) const
    {
        if (_accepted)
            _accepted(socket);
    }

}  // namespace Rt2::Sockets
