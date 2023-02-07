#include <cstdio>
#include "Sockets/PlatformSocket.h"
#include "gtest/gtest.h"
#include "Utils/Console.h"


using namespace Rt2;


GTEST_TEST(Sockets, AlwaysTrue)
{
    Sockets::initialize();

    Sockets::HostInfo inf;
    Sockets::getHostInfo(inf, "google.com");

    for (const auto& [name, address, family, type, protocol] : inf)
    {
        Console::writeLine("==============");
        Console::writeLine("Host          : ", name);
        Console::writeLine("Address       : ", address);
        Console::writeLine("Protocol      : ", toString(protocol));
        Console::writeLine("AddressFamily : ", toString(family));
        Console::writeLine("SocketType    : ", toString(type));
        Console::writeLine("==============");
        Console::writeLine("");
    }
}
