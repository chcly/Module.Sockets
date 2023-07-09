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
#include "Sockets/ServerThread.h"
#include "Sockets/ServerSocket.h"

namespace Rt2::Sockets
{
    using Accept = Accept;

    ServerThread::ServerThread(ServerSocket* owner) :
        _owner(owner)
    {
    }

    const PlatformSocket& ServerThread::socket() const
    {
        RT_GUARD_CHECK_RET(_owner, InvalidSocket)
        return _owner->socket();
    }

    void ServerThread::update()
    {
        int n = 0, m = 0;
        while (isRunning())
        {
            if (Net::poll(socket(), 0, Read))
            {
                Connection client;
                if (const PlatformSocket sock = Net::accept(socket(), client);
                    sock != InvalidSocket)
                {
                    // clang-format off
                    Thread::StandardThread
                    {
                        [&n](const Accept& accept, const PlatformSocket& s)
                        {
                            if (accept)  accept(s);
                            Net::close(s);
                            --n;
                        },
                        _owner->accept(),
                        sock,
                    }.detach();
                    // clang-format on
                    ++n;
                }
            }
        }

        while (n > 0 && m < 100)
        {
            Thread::Thread::sleep(10);
            ++m;
        }
        RT_GUARD_CHECK_VOID(n == 0 && m == 0)
    }
}  // namespace Rt2::Sockets
