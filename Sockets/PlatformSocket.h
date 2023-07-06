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
#include "Utils/Definitions.h"
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
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
#include "Utils/Json.h"
#include "Utils/String.h"

namespace Rt2::Sockets
{
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
    using PlatformSocket                   = SOCKET;
    constexpr PlatformSocket InvalidSocket = INVALID_SOCKET;
#else
    using PlatformSocket                   = int;
    constexpr PlatformSocket InvalidSocket = -1;
#endif
    constexpr int MaxBufferSize = 0x7FFFFF;

    namespace Default
    {
        constexpr size_t IoBufferSize  = 0x800;
        constexpr int    SocketTimeOut = 1000;
    }  // namespace Default

    class Connection;

    using SocketInputAddress = sockaddr_in;
    using InputAddress       = in_addr;

    enum Status
    {
        DoneStatus  = -2,
        ErrorStatus = -1,
        OkStatus    = 0,
    };

    enum AddressFamily
    {
        AddressFamilyUnknown = AF_UNSPEC,
        AddressFamilyUnix    = AF_UNIX,
        AddressFamilyINet    = AF_INET,
    };

    enum Protocol
    {
        ProtocolUnknown = PF_UNSPEC,
        ProtocolIpTcp   = IPPROTO_TCP,  // Transmission Control Protocol
        ProtocolIpUdp   = IPPROTO_UDP,  // User Datagram Protocol
        ProtocolIpRaw   = IPPROTO_RAW,  // Raw IP packets.
    };

    enum SocketType
    {
        SocketUnknown  = -0xFF,
        SocketStream   = SOCK_STREAM,
        SocketDatagram = SOCK_DGRAM,
        SocketRaw      = SOCK_RAW,
    };

    enum SocketOption
    {
        Blocking          = -0xFF,
        Debug             = SO_DEBUG,
        Broadcast         = SO_BROADCAST,
        ReuseAddress      = SO_REUSEADDR,
        KeepAlive         = SO_KEEPALIVE,
        DoNotRoute        = SO_DONTROUTE,
        SendBufferSize    = SO_SNDBUF,
        ReceiveBufferSize = SO_RCVBUF,
        ReceiveTimeout    = SO_RCVTIMEO,
        SendTimeout       = SO_SNDTIMEO,
    };

    enum PollMode
    {
        Read      = 0x01,
        Write     = 0x10,
        ReadWrite = Read | Write
    };

    struct Host
    {
        String        name;
        String        address;
        AddressFamily family{AddressFamilyINet};
        SocketType    type{SocketStream};
        Protocol      protocol{ProtocolUnknown};
    };

    using HostInfo = std::vector<Host>;

    class HostEnumerator
    {
    private:
        HostInfo     _hosts;
        const String _name;

    public:
        explicit HostEnumerator(const String& hostName);

        const HostInfo& hosts();

        void log(OStream& out) const;

        bool match(Host&         dest,
                   AddressFamily family,
                   Protocol      protocol = ProtocolUnknown,
                   SocketType    type     = SocketUnknown) const;
    };

    class Connection
    {
    private:
        SocketInputAddress _inp{};

    public:
        Connection() = default;

        SocketInputAddress& input();

        String address() const;

        uint16_t port() const;
    };

    class Net
    {
    public:
        static PlatformSocket create(
            AddressFamily address,
            SocketType    type,
            Protocol      protocol = ProtocolUnknown,
            bool          block    = false);

        static void close(
            const PlatformSocket& sock);

        static Status connect(
            const PlatformSocket& sock,
            const String&         ipv4,
            uint16_t              port);

        static PlatformSocket connect(
            const Host& host,
            uint16_t    port);

        static Status bind(
            const PlatformSocket& sock,
            SocketInputAddress&   addr);

        static Status listen(
            const PlatformSocket& sock,
            int32_t               backlog);

        static PlatformSocket accept(
            const PlatformSocket& sock);

        static PlatformSocket accept(
            const PlatformSocket& sock,
            SocketInputAddress&   dest);

        static PlatformSocket accept(
            const PlatformSocket& sock,
            Connection&           result);

        static bool poll(
            const PlatformSocket& sock,
            int                   timeout = 0,
            int                   mode    = ReadWrite);

        static Status readSocket(
            const PlatformSocket& sock,
            char*                 dest,
            int                   destSizeInBytes,
            int&                  bytesRead,
            int                   timeout = 100);

        static int writeSocket(
            const PlatformSocket& sock,
            const void*           ptr,
            size_t                sizeInBytes,
            int                   timeout = 100);

        static Status setOption(
            const PlatformSocket& sock,
            SocketOption          option,
            bool                  val);

        static Status setOption(
            const PlatformSocket& sock,
            SocketOption          option,
            int                   val);

        static bool optionBool(const PlatformSocket& sock,
                               SocketOption          option);
        static int  optionInt(const PlatformSocket& sock,
                              SocketOption          option);

        static void ensureInitialized();

        class Utils
        {
        public:
            static void setBlocking(PlatformSocket sock, bool val);

            static uint32_t asciiToNetworkIpV4(const String& inp);

            static String networkToAsciiIpV4(const uint32_t& inp);

            static uint32_t networkToHostLong(const uint32_t& inp);

            static uint16_t hostToNetworkShort(const uint16_t& inp);

            static uint16_t networkToHostShort(const uint16_t& inp);

            static bool getHostInfo(HostInfo& inf, const String& name);

            static Host iNetHost(const HostInfo& inf);

            static Json::Dictionary toJson(const Host& host);

            static void toJson(const Json::MixedArray& arr, const HostInfo& info);

            static String toString(const HostInfo& info);

            static String toString(const Host& info);

            static void toStream(OStream& dest, const HostInfo& info);

            static void toStream(OStream& dest, const Host& info);

            static String toString(const AddressFamily& addressFamily);

            static String toString(const Protocol& protocolFamily);

            static String toString(const SocketType& socketType);

            static void constructInputAddress(
                SocketInputAddress& dest,
                AddressFamily       addressFamily,
                uint16_t            port,
                const String&       address);
        };

        class Error
        {
        public:
            static void log();

            static void log(OStream& out);

            [[noreturn]] static void error();
        };

    private:
        static Status setOption(
            const PlatformSocket& sock,
            SocketOption          option,
            const void*           value,
            size_t                valueSize);

        static Status getOption(
            const PlatformSocket& sock,
            SocketOption          option,
            void*                 dest,
            int&                  size);
    };

    inline SocketInputAddress& Connection::input()
    {
        return _inp;
    }

    inline String Connection::address() const
    {
        return Net::Utils::networkToAsciiIpV4(_inp.sin_addr.s_addr);
    }

    inline uint16_t Connection::port() const
    {
        return Net::Utils::networkToHostShort(_inp.sin_port);
    }

}  // namespace Rt2::Sockets
