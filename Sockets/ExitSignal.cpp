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
#include "Sockets/ExitSignal.h"
#include <csignal>

namespace Rt2::Sockets
{
    ExitSignal* ExitSignal::_signal = nullptr;

    void ExitSignal::signalMethod(int)
    {
        if (_signal)
            _signal->_signaled = true;
    }

    ExitSignal::ExitSignal()
    {
        _signal = this;
        (void)signal(SIGINT, signalMethod);
        (void)signal(SIGTERM, signalMethod);
    }

    ExitSignal::~ExitSignal()
    {
        _signal = nullptr;
    }

}  // namespace Rt2::Sockets
