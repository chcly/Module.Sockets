#include <cstdio>
#include "Sockets/ClientSocket.h"
#include "Sockets/PlatformSocket.h"
#include "Sockets/ServerSocket.h"
#include "Sockets/SocketStream.h"
#include "Utils/Console.h"
#include "gtest/gtest.h"

using namespace Rt2;

GTEST_TEST(Sockets, Sock_001)
{
    using namespace Sockets;

    Socket sock;
    sock.create();
    EXPECT_TRUE(sock.isValid());
    EXPECT_EQ(sock.protocol(), ProtocolUnknown);
    EXPECT_EQ(sock.family(), AddressFamilyINet);
    EXPECT_EQ(sock.type(), SocketStream);

    sock.close();
    EXPECT_FALSE(sock.isValid());
}

GTEST_TEST(Sockets, Sock_002)
{
    using namespace Sockets;

    Socket sock;
    sock.create();
    EXPECT_TRUE(sock.isValid());

    EXPECT_FALSE(sock.reuseAddress());
    sock.setReuseAddress(true);
    EXPECT_TRUE(sock.reuseAddress());

    EXPECT_FALSE(sock.keepAlive());
    sock.setKeepAlive(true);
    EXPECT_TRUE(sock.keepAlive());

    EXPECT_FALSE(sock.isDebug());
    sock.setDebug(true);
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
    EXPECT_TRUE(sock.isDebug());
#else
    EXPECT_FALSE(sock.isDebug());  // will fail if run with privileged access
#endif

    EXPECT_TRUE(sock.isRouting());
    sock.setRoute(false);
    EXPECT_FALSE(sock.isRouting());

#if RT_PLATFORM == RT_PLATFORM_WINDOWS
    EXPECT_EQ(sock.maxSendBuffer(), 0x10000);
    EXPECT_EQ(sock.maxReceiveBuffer(), 0x10000);

    sock.setMaxSendBuffer(0x400);
    sock.setMaxReceiveBuffer(0x400);

    EXPECT_EQ(sock.maxSendBuffer(), 0x400);
    EXPECT_EQ(sock.maxReceiveBuffer(), 0x400);
#endif

    EXPECT_EQ(sock.sendTimeout(), 0);
    EXPECT_EQ(sock.receiveTimeout(), 0);

    sock.setSendTimeout(1234568);
    sock.setReceiveTimeout(1234568);

    EXPECT_EQ(sock.sendTimeout(), 1234568);
    EXPECT_EQ(sock.receiveTimeout(), 1234568);

    sock.close();
    EXPECT_FALSE(sock.isValid());
}

GTEST_TEST(Sockets, GetHostInfo)
{
    using namespace Sockets;
    
    const auto en = HostEnumerator("github.com");

    Host github;
    EXPECT_TRUE(en.match(github, AddressFamilyINet));

    HostEnumerator he("github.com");
    EXPECT_FALSE(he.hosts().empty());
    he.log(std::cout);
}

GTEST_TEST(Sockets, GetHeaders)
{
    using namespace Sockets;

    const auto en = HostEnumerator("google.com");

    Host google;
    EXPECT_TRUE(en.match(google, AddressFamilyINet));

    const PlatformSocket sock = Net::connect(google, 80);
    EXPECT_NE(sock, InvalidSocket);

    OutputSocketStream bs(sock);
    bs.write("HEAD / HTTP/1.1\r\n\r\n");

    InputSocketStream is(sock);
    is.setBlockSize(1124);
    Console::hexdump(is.string());
}

GTEST_TEST(Sockets, LocalLink)
{
    using namespace Sockets;
    bool connected = false;

    ServerSocket ss("127.0.0.1", 8080);
    ss.connect(
        [&connected, &ss](const PlatformSocket& sock)
        {
            connected = true;
            InputSocketStream si(sock);
            si.setTimeout(2500);

            String msg;
            si.get(msg);
            EXPECT_EQ(msg, "Hello");

            si.get(msg);
            EXPECT_EQ(msg, "World");

            EXPECT_TRUE(si.eof());
            ss.stop();
        });
    const ClientSocket cs("127.0.0.1", 8080);
    cs.write("Hello World");
    ss.run();
    EXPECT_TRUE(connected);
}
