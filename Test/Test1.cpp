#include <cstdio>
#include "Sockets/ClientSocket.h"
#include "Sockets/PlatformSocket.h"
#include "Sockets/ServerSocket.h"
#include "Utils/Console.h"
#include "gtest/gtest.h"


using namespace Rt2;

GTEST_TEST(Sockets, Server_001)
{
    Sockets::ServerSocket ss("127.0.0.1", 8080);
    EXPECT_TRUE(ss.isOpen());
    ss.start();


    ss.onMessageReceived(
        [](const String& msg)
        {
            Console::writeLine("msg=", msg);
            EXPECT_EQ(msg, "Hello1");
        });

    const Sockets::ClientSocket c1("127.0.0.1", 5000);
    EXPECT_TRUE(c1.isOpen());
    c1.write("Hello1");
    const Sockets::ClientSocket c2("127.0.0.1", 5000);
    EXPECT_TRUE(c2.isOpen());
    c2.write("Hello1");
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
