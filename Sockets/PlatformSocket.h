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

#ifdef _WIN32
    #include <WinSock2.h>
    #include "WS2tcpip.h"
#else
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/signal.h>
    #include <sys/socket.h>
    #include <sys/time.h>
#endif
#include <cstdint>
#include "Utils/String.h"

namespace Rt2::Sockets::Net
{
    constexpr int MaxBufferSize = 0x7FFFFF;

    class Connection;

    using SocketInputAddress = sockaddr_in;
    using InputAddress       = in_addr;

#ifdef _WIN32
    using Socket                   = SOCKET;
    constexpr Socket InvalidSocket = INVALID_SOCKET;
#else
    using Socket                   = int;
    constexpr Socket InvalidSocket = -1;
#endif

    enum Status
    {
        Done  = -2,
        Error = -1,
        Ok    = 0,
    };

    enum AddressFamily
    {
        AddrUnspecified = AF_UNSPEC,
        AddrUnix        = AF_UNIX,
        AddrINet        = AF_INET,
    };

    enum ProtocolFamily
    {
        ProtoUnspecified = PF_UNSPEC,
        ProtoIpTcp       = IPPROTO_TCP,  // Transmission Control Protocol
        ProtoIpUdp       = IPPROTO_UDP,  // User Datagram Protocol
        ProtoIpRaw       = IPPROTO_RAW,  // Raw IP packets.
    };

    enum SocketType
    {
        UnknownSocket = -20,
        Stream        = SOCK_STREAM,
        Datagram      = SOCK_DGRAM,
        Raw           = SOCK_RAW,
        Rdm           = SOCK_RDM,
        Seq           = SOCK_SEQPACKET,

    };

    String toString(const AddressFamily& addressFamily);

    String toString(const ProtocolFamily& protocolFamily);

    String toString(const SocketType& socketType);

    enum ProtocolLevel
    {
        SocketLevel = SOL_SOCKET,
    };

    enum SocketOption
    {
        Debug             = SO_DEBUG,
        Broadcast         = SO_BROADCAST,
        ReuseAddress      = SO_REUSEADDR,
        KeepAlive         = SO_KEEPALIVE,
        Linger            = SO_LINGER,
        SendBufferSize    = SO_SNDBUF,
        ReceiveBufferSize = SO_RCVBUF,
        DoNotRoute        = SO_DONTROUTE,
        ReceiveWaitMin    = SO_RCVLOWAT,
        ReceiveWaitMax    = SO_RCVTIMEO,
        SendWaitMin       = SO_RCVLOWAT,
        SendWaitMax       = SO_RCVTIMEO,
    };

    extern void ensureInitialized();

    extern void initialize();

    extern void finalize();

    extern Socket create(
        AddressFamily  addressFamily,
        SocketType     type,
        ProtocolFamily protocolFamily = ProtoUnspecified,
        bool           block          = false);

    extern void close(const Socket& sock);

    extern Status setOption(
        Socket&       sock,
        ProtocolLevel protocolLevel,
        SocketOption  optionType,
        const void*   option,
        size_t        optionSize);

    extern void constructInputAddress(
        SocketInputAddress& dest,
        AddressFamily       addressFamily,
        uint16_t            port,
        const String&       address);

    extern uint32_t AsciiToNetworkIpV4(const String& inp);

    extern String NetworkToAsciiIpV4(const uint32_t& inp);

    extern uint32_t NetworkToHostLong(const uint32_t& inp);

    extern uint16_t HostToNetworkShort(const uint16_t& inp);

    extern uint16_t NetworkToHostShort(const uint16_t& inp);

    extern Status bind(const Socket& sock, SocketInputAddress& addr);

    extern Status listen(const Socket& sock, int32_t backlog);

    extern Status connect(const Socket& sock, const String& str, uint16_t port);

    extern Socket accept(const Socket& sock);

    extern Socket accept(const Socket& sock, SocketInputAddress& dest);

    extern Socket accept(const Socket& sock, Connection& result);

    extern Status readBuffer(const Socket& sock, char* buffer, int bufLen, int& bytesRead);

    extern int writeBuffer(const Socket& sock, const void* buffer, size_t bufLen);

    struct Host
    {
        String         name;
        String         address;
        AddressFamily  family{AddrINet};
        SocketType     type{Stream};
        ProtocolFamily protocol{ProtoUnspecified};
    };

    using HostInfo = std::vector<Host>;

    bool getHostInfo(HostInfo& inf, const String& name);

    Host INetHost(const HostInfo& inf);

    extern String toString(const HostInfo& info);

}  // namespace Rt2::Sockets::Net
