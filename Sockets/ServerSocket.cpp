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
#include "Threads/Mutex.h"
#include "Threads/Task.h"
#include "Threads/Thread.h"
#include "Utils/Exception.h"

namespace Rt2::Sockets
{

    class ServerThread final : public Threads::Thread
    {
    private:
        bool                   _status{true};
        const ServerSocket*    _socket;
        mutable Threads::Mutex _sec;
        int                    _maxConnect{0};
        int                    _connect{0};

    private:
        void readSocket(Net::Socket socket) const;

        void acceptConnections();

        int update() override;

    public:
        explicit ServerThread(const ServerSocket* parent, int maxConnections) :
            _socket(parent),
            _maxConnect(maxConnections)
        {
        }

        ~ServerThread() override
        {
            kill();
            join();
        }

        void kill()
        {
            ScopeLockMtx(_sec);
            _status = false;
        }
    };

    void ServerThread::readSocket(const Net::Socket socket) const
    {
        SocketInputStream<72> in(socket);

        OutputStringStream oss;
        while (!in.eof())
        {
            String message;
            in >> message;
            oss << message;
            if (!in.eof())
                oss << ' ';
        }

        ScopeLockMtx(_sec);
        _socket->dispatch(oss.str());
    }

    void ServerThread::acceptConnections()
    {
        Net::Connection client;
        if (const Net::Socket sock = Net::accept(_socket->_server, client);
            sock != Net::InvalidSocket)
        {
            Threads::Task::start(
                [=]
                {
                    readSocket(sock);
                },
                [=]
                {
                    Net::close(sock);
                    _connect++;
                });
        }
        else
            std::this_thread::yield();
    }

    int ServerThread::update()
    {
        if (_maxConnect > 0)
        {
            while (_connect < _maxConnect)
                acceptConnections();
        }
        else
        {
            while (_status)
                acceptConnections();
        }

        return 0;
    }

    void ServerSocket::dispatch(const String& msg) const
    {
        if (_message)
            _message(msg);
    }

    ServerSocket::ServerSocket(const String& ipv4, const uint16_t port, const int maxConnections) :
        _maxConnect(maxConnections)
    {
        Net::ensureInitialized();
        open(ipv4, port);
    }

    ServerSocket::~ServerSocket()
    {
        delete _main;
        _main = nullptr;

        if (_server)
        {
            Net::close(_server);
            _server = Net::InvalidSocket;
        }
    }

    void ServerSocket::onMessageReceived(const MessageFunction& function)
    {
        Threads::CriticalSectionLock sl(&_sec);
        _message = function;
    }

    void ServerSocket::start()
    {
        if (!_main)
        {
            _main = new ServerThread(this, _maxConnect);
            _main->start();
        }
    }

    void ServerSocket::open(const String& ipv4, uint16_t port, int32_t backlog)
    {
        using namespace Net;

        try
        {
            _server = create(AddrINet, Stream, ProtoUnspecified);
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
