#include <cstdio>
#include "Sockets/ClientSocket.h"
#include "Sockets/PlatformSocket.h"
#include "Sockets/ServerSocket.h"
#include "Sockets/SocketStream.h"
#include "Utils/Console.h"
#include "gtest/gtest.h"

using namespace Rt2;

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

    const Socket sock = create(AddrINet, Stream, ProtoUnspecified, true);
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
            Console::hexdump(buffer, (uint32_t)br);
        }
    }
}

GTEST_TEST(Sockets, LocalLink)
{
    bool connected = false;

    Sockets::ServerSocket ss("127.0.0.1", 8080);
    ss.connect(
        [&connected](const Sockets::Net::Socket& sock)
        {
            connected = true;
            Sockets::SocketInputStream si(sock);

            String msg;
            si >> msg;
            Console::println(msg);
            Sockets::ServerSocket::signal();
        });
    ss.start();

    Sockets::ClientSocket cs("127.0.0.1", 8080);
    cs.write("Hello ");
    ss.waitSignaled();
    EXPECT_TRUE(connected);
}
