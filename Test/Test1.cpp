#include <cstdio>
#include "Sockets/ClientSocket.h"
#include "Sockets/PlatformSocket.h"
#include "Sockets/ServerSocket.h"
#include "Utils/Console.h"
#include "gtest/gtest.h"


using namespace Rt2;

void stall(int ms)
{
    while (ms-- > 0)
        ;
}



GTEST_TEST(Sockets, Server_001)
{
    constexpr U16 port = 5555;
    constexpr int max  = 16;

    Sockets::ServerSocket ss("127.0.0.1", port, max);
    EXPECT_TRUE(ss.isOpen());
    ss.start();


    int nr = 0;
    ss.onMessageReceived(
        [&nr](const String& msg)
        {
            ++nr;
            const String& exp = Su::join("Hello from client #", nr);
            Console::writeLine(exp);
            EXPECT_EQ(msg, exp);
        });

    for (int i = 0; i < max; ++i)
    {
        const Sockets::ClientSocket client("127.0.0.1", port);
        EXPECT_TRUE(client.isOpen());
        client.write(Su::join("Hello from client #", i + 1));
    }
}


GTEST_TEST(Sockets, AlwaysTrue)
{
    using namespace Sockets::Net;
    ensureInitialized();

    HostInfo inf;
    getHostInfo(inf, "github.com");

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
