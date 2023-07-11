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
#include <iostream>
#include "Utils/Console.h"

namespace Rt2::Sockets
{
    ExitSignal*        ExitSignal::_signal   = nullptr;
    ExitSignal::Signal ExitSignal::_function = nullptr;
    SignalHandler      ExitSignal::_prevInt  = nullptr;
    SignalHandler      ExitSignal::_prevTerm = nullptr;

    void ExitSignal::signalMethod(const int sig)
    {
        if (_signal)
            _signal->_signaled = true;
        if (_function)
            _function();

        std::cin.clear();
        std::cin.putback('\n');

        (void)std::signal(sig, signalMethod);
    }

    ExitSignal::ExitSignal()
    {
        _signal   = this;
        _prevInt  = std::signal(SIGINT, signalMethod);
        _prevTerm = std::signal(SIGTERM, signalMethod);
    }

    ExitSignal::~ExitSignal()
    {
        _signal = nullptr;

        (void)std::signal(SIGINT, _prevInt);
        (void)std::signal(SIGTERM, _prevTerm);
    }

    void ExitSignal::signal()
    {
        (void)std::raise(SIGINT);
    }

    void ExitSignal::bind(const Signal& fn)
    {
        _function = fn;
    }
}  // namespace Rt2::Sockets
