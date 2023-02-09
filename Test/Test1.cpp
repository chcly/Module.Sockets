#include <cstdio>
#include "Sockets/ClientSocket.h"
#include "Sockets/PlatformSocket.h"
#include "Sockets/ServerSocket.h"
#include "Sockets/SocketStream.h"
#include "Utils/Console.h"
#include "gtest/gtest.h"

using namespace Rt2;

GTEST_TEST(Sockets, Server_001)
{
    constexpr U16 port = 5555;
    constexpr int max  = 5;

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

GTEST_TEST(Sockets, GetHostInfo)
{
    using namespace Sockets::Net;
    ensureInitialized();

    HostInfo inf;
    getHostInfo(inf, "github.com");
    EXPECT_FALSE(inf.empty());

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

GTEST_TEST(Sockets, GetHeaders)
{
    using namespace Sockets::Net;
    ensureInitialized();

    constexpr const char* name = "google.com";

    HostInfo inf;
    getHostInfo(inf, name);
    EXPECT_FALSE(inf.empty());

    Host inet = INetHost(inf);
    EXPECT_FALSE(inet.address.empty());

    Console::writeLine(toString(inf));

    const Socket sock = create(AddrINet, Stream);
    EXPECT_NE(sock, InvalidSocket);

    const int res = connect(sock, inet.address, 80);
    EXPECT_EQ(res, Ok);

    OutputStringStream oss;
    oss << "HEAD / HTTP/1.1\r\n\r\n";

    const String str = oss.str();
    writeBuffer(sock, str.c_str(), str.size());

    char buffer[1025]{};

    Sockets::SocketStream ss(sock);
    while (!ss.eof())
    {
        if (const size_t br = ss.read(buffer, 1024);
            br <= 1024)
        {
            buffer[br] = 0;
            printBuffer(buffer, (uint32_t)br);
        }
    }
}
