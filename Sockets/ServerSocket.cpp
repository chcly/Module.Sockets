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
#include "Sockets/ServerThread.h"
#include "Thread/Runner.h"
#include "Utils/Exception.h"

namespace Rt2::Sockets
{
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
        RT_GUARD_CHECK_VOID(_main)
        while (_running)
            Thread::Thread::yield();
        destroy();
    }

    void ServerSocket::run(const Update& up)
    {
        RT_GUARD_CHECK_VOID(_main && up)
        while (_running) 
            up();
        destroy();
    }

    void ServerSocket::stop()
    {
        _running = false;
    }

    void ServerSocket::connect(const Accept& onAccept)
    {
        _accepted = onAccept;
    }

    Accept ServerSocket::accept()
    {
        return _accepted;
    }

}  // namespace Rt2::Sockets
