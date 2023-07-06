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
#include <functional>
#include "Sockets/Socket.h"
#include "Thread/SharedValue.h"

namespace Rt2::Sockets
{
    class ServerThread;
    using ConnectionAccepted = std::function<void(const PlatformSocket& con)>;

    class ServerSocket final : public Socket
    {
    private:
        friend class ServerThread;
        Thread::Mutex      _mutex;
        ServerThread*      _main{nullptr};
        ConnectionAccepted _accepted;
        bool               _running{false};

    public:
        ServerSocket(const String& ipv4, uint16_t port, uint16_t backlog = 0x100);
        ~ServerSocket() override;

        void run();

        void stop();

        void connect(const ConnectionAccepted& onAccept);

        static void signal();

    private:
        void connected(const PlatformSocket& socket) const;

        void open(const String& ipv4, uint16_t port, uint16_t backlog);
        
        void start();

        void destroy();
    };

}  // namespace Rt2::Sockets
