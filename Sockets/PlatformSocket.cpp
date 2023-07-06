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
#include "Sockets/PlatformSocket.h"
#include <cmath>
#include <cstdio>
#include <iostream>
#include "Connection.h"
#include "Thread/Thread.h"
#include "Utils/Char.h"
#include "Utils/Definitions.h"
#include "Utils/Exception.h"
#include "Utils/Json.h"
#include "Utils/TextStreamWriter.h"

#if RT_PLATFORM != RT_PLATFORM_WINDOWS
    #include <fcntl.h>
    #include <poll.h>
    #include <sys/select.h>
    #include <unistd.h>
    #include <cstring>
#endif

// #define VERBOSE_DEBUG
namespace Rt2::Sockets
{
    PlatformSocket Net::create(
        const AddressFamily address,
        const SocketType    type,
        const Protocol      protocol,
        const bool          block)
    {
        const PlatformSocket sock = socket(address, type, protocol);
        return sock;
    }

    void Net::close(const PlatformSocket& sock)
    {
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
        if (closesocket(sock) != 0)
            Error::log();
#else
        ::close(sock);
#endif
    }

    Status Net::connect(
        const PlatformSocket& sock,
        const String&         ipv4,
        const uint16_t        port)
    {
        SocketInputAddress inp = {};
        inp.sin_family         = AF_INET;
        inp.sin_port           = Utils::hostToNetworkShort(port);
        inp.sin_addr.s_addr    = Utils::asciiToNetworkIpV4(ipv4);
        return (Status)::connect(sock, (const sockaddr*)&inp, sizeof(sockaddr));
    }

    PlatformSocket Net::connect(const Host& host, const uint16_t port)
    {
        try
        {
            const PlatformSocket sock = create(host.family, host.type, host.protocol, true);
            if (sock != InvalidSocket)
            {
                if (const Status st = connect(sock, host.address, port); st != OkStatus)
                {
                    Error::log(std::cout);
                    close(sock);
                    return InvalidSocket;
                }
            }
            return sock;
        }
        catch (...)
        {
        }
        return InvalidSocket;
    }

    Status Net::bind(
        const PlatformSocket& sock,
        SocketInputAddress&   addr)
    {
        return (Status)::bind(sock, (sockaddr*)&addr, sizeof(sockaddr));
    }

    Status Net::listen(
        const PlatformSocket& sock,
        const int32_t         backlog)
    {
        return (Status)::listen(sock, backlog);
    }

    PlatformSocket Net::accept(
        const PlatformSocket& sock)
    {
        SocketInputAddress unused = {};

        int sz = sizeof(SocketInputAddress);
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
        return ::accept(sock, (sockaddr*)&unused, &sz);
#else
        return ::accept(sock, (sockaddr*)&unused, (socklen_t*)&sz);
#endif
    }

    PlatformSocket Net::accept(
        const PlatformSocket& sock,
        SocketInputAddress&   dest)
    {
        dest         = {};
        socklen_t sz = sizeof(SocketInputAddress);
        return ::accept(sock, (sockaddr*)&dest, &sz);
    }

    PlatformSocket Net::accept(
        const PlatformSocket& sock,
        Connection&           result)
    {
        return accept(sock, result.input());
    }

    bool Net::poll(
        const PlatformSocket& sock,
        const int             timeout,
        const int             mode)
    {
        RT_GUARD_CHECK_RET(sock != InvalidSocket, false)
        fd_set set = {};
        FD_ZERO(&set);
        FD_CLR(sock, &set);
        FD_SET(sock, &set);

        const int nfd = (int)(sock + 1);

        const timeval tv{0, (long)timeout * 1000};

        timeval* tp = nullptr;
        if (timeout >= 0) tp = const_cast<timeval*>(&tv);

        if (mode == ReadWrite)
            return select(nfd, &set, &set, nullptr, tp) > 0;
        if (mode == Read)
            return select(nfd, &set, nullptr, nullptr, tp) > 0;
        if (mode == Write)
            return select(nfd, nullptr, &set, nullptr, tp) > 0;
        return false;
    }

    Status Net::readSocket(
        const PlatformSocket& sock,
        char*                 dest,
        const int             destSizeInBytes,
        int&                  bytesRead,
        const int             timeout)
    {
        RT_GUARD_CHECK_RET(dest, ErrorStatus)
        RT_GUARD_CHECK_RET(destSizeInBytes < MaxBufferSize, ErrorStatus)

        bytesRead = 0;
        if (poll(sock, timeout, Read))
        {
            if (const int rl = recv(sock, dest, destSizeInBytes, 0);
                rl > 0 && rl <= destSizeInBytes)
            {
                bytesRead = rl;
                dest[rl]  = 0;
                if (rl < destSizeInBytes)
                    return DoneStatus;
                return OkStatus;
            }
        }
        return DoneStatus;
    }

    int Net::writeSocket(
        const PlatformSocket& sock,
        const void*           ptr,
        const size_t          sizeInBytes,
        const int             timeout)
    {
        RT_GUARD_CHECK_RET(ptr, -1)
        RT_GUARD_CHECK_RET(sizeInBytes < MaxBufferSize, -1)

        int rc = -1;
        if (poll(sock, timeout, Write))
        {
            rc = send(sock, (const char*)ptr, (int)sizeInBytes, 0);
            Thread::Thread::yield();
        }
        return rc;
    }

    Status Net::setOption(
        const PlatformSocket& sock,
        const SocketOption    option,
        const bool            val)
    {
        const int setVal = val ? 1 : 0;
        Status    st     = OkStatus;
        switch (option)
        {
        case ReuseAddress:
        case Debug:
        case KeepAlive:
        case DoNotRoute:
        case Broadcast:
            st = setOption(sock, option, &setVal, sizeof(int));
            break;
        case Blocking:
            Utils::setBlocking(sock, val);
            break;
        case SendBufferSize:
        case SendTimeout:
        case ReceiveBufferSize:
        case ReceiveTimeout:
        default:
            break;
        }

        if (st != OkStatus)
            Error::log();
        return st;
    }

    void splitMilliseconds(const int ms, long& seconds, long& microseconds)
    {
        const long a = ms * 1000;
        microseconds = a % 1000000;
        seconds      = (a - microseconds) / 1000000;
    }

    void joinMilliseconds(int& milliseconds, const long& seconds, const long& microseconds)
    {
        milliseconds = 1000 * seconds + long(double(microseconds) / 1000.0);
    }

    Status Net::setOption(
        const PlatformSocket& sock,
        const SocketOption    option,
        const int             val)
    {
        Status st = OkStatus;
        switch (option)
        {
        case ReuseAddress:
        case Debug:
        case KeepAlive:
        case DoNotRoute:
        case Broadcast:
        {
            const int sv = val != 0 ? 1 : 0;

            st = setOption(sock, option, &sv, sizeof(int));
            break;
        }
        case Blocking:
            Utils::setBlocking(sock, val != 0);
            break;
        case SendBufferSize:
        case ReceiveBufferSize:
            st = setOption(sock, option, &val, sizeof(int));
            break;
        case SendTimeout:
        case ReceiveTimeout:
        {
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
            st = setOption(sock, option, &val, sizeof(int));
#else
            // clang-format off
            timeval tv = {};
            splitMilliseconds(val, tv.tv_sec, tv.tv_usec);
            st = setOption(sock, option, &tv, sizeof(timeval));
            // clang-format on
#endif

            break;
        }
        default:
            break;
        }

        if (st != OkStatus)
            Error::log();
        return st;
    }

    bool Net::optionBool(const PlatformSocket& sock, const SocketOption option)
    {
        int    get = 0;
        int    sz  = sizeof(int);
        Status st  = OkStatus;
        switch (option)
        {
        case ReuseAddress:
        case Debug:
        case KeepAlive:
        case DoNotRoute:
        case Broadcast:
            st = getOption(sock, option, &get, sz);
            break;
        case Blocking:
        case SendBufferSize:
        case SendTimeout:
        case ReceiveBufferSize:
        case ReceiveTimeout:
        default:
            break;
        }

        if (st != OkStatus)
        {
            Error::log();
        }
        return get != 0;
    }

    int Net::optionInt(const PlatformSocket& sock, const SocketOption option)
    {
        int    get = 0;
        int    sz  = sizeof(int);
        Status st  = OkStatus;
        switch (option)
        {
        case ReuseAddress:
        case Debug:
        case KeepAlive:
        case DoNotRoute:
        case Broadcast:
        case Blocking:
        case SendBufferSize:
        case ReceiveBufferSize:
            st = getOption(sock, option, &get, sz);
            break;
        case ReceiveTimeout:
        case SendTimeout:
        {
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
            st = getOption(sock, option, &get, sz);
#else
            // clang-format off
            timeval tv = {};
            sz = sizeof(timeval);
            st = getOption(sock, option, &tv, sz);
            joinMilliseconds(get, tv.tv_sec, tv.tv_usec);
            // clang-format on
#endif
            break;
        }
        default:
            break;
        }

        if (st != OkStatus)
            Error::log();
        return get;
    }

    Status Net::setOption(
        const PlatformSocket& sock,
        SocketOption          option,
        const void*           value,
        size_t                valueSize)
    {
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
        return (Status)setsockopt(
            sock,
            SOL_SOCKET,
            (int)option,
            (const char*)value,
            (socklen_t)valueSize);
#else
        return (Status)setsockopt(
            sock,
            SOL_SOCKET,
            (int)option,
            (const void*)value,
            (socklen_t)valueSize);

#endif
    }

    Status Net::getOption(
        const PlatformSocket& sock,
        const SocketOption    option,
        void*                 dest,
        int&                  size)
    {
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
        return (Status)getsockopt(sock, SOL_SOCKET, (int)option, (char*)dest, &size);
#else
        socklen_t wrap;
        auto      rc = (Status)getsockopt(
            sock,
            SOL_SOCKET,
            (int)option,
            dest,
            &wrap);
        size = (int)wrap;
        return rc;
#endif
    }

    void Net::Utils::setBlocking(
        PlatformSocket sock,
        const bool     val)
    {
        if (sock != InvalidSocket)
        {
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
            u_long block = val ? 0 : 1;
            if (ioctlsocket(sock, FIONBIO, &block) != 0)
                Error::log();
#else
            int cfl = fcntl(sock, F_GETFL);
            if (!val)
                cfl |= O_NONBLOCK;
            else
                cfl &= ~O_NONBLOCK;
            fcntl(sock, F_SETFL, cfl);
#endif
        }
    }

    void Net::Utils::constructInputAddress(
        SocketInputAddress& dest,
        const AddressFamily addressFamily,
        const uint16_t      port,
        const String&       address)
    {
        dest = {};

#ifdef WIN32
        dest.sin_family = (ADDRESS_FAMILY)addressFamily;
#else
        dest.sin_family = (int)addressFamily;
#endif
        dest.sin_port        = hostToNetworkShort(port);
        dest.sin_addr.s_addr = asciiToNetworkIpV4(address);
    }

    bool isOnlyDecimal(const String& inp)
    {
        if (inp.size() > 3)
            return false;

        for (const char ch : inp)
        {
            if (!isDecimal(ch))
                return false;
        }

        const int valCheck = Char::toInt32(inp);
        return valCheck <= 255;
    }

    uint32_t Net::Utils::asciiToNetworkIpV4(const String& inp)
    {
        if (inp.empty())  // use current
            return 0;

        StringArray sa;
        StringUtils::splitRejectEmpty(sa, inp, '.');

        if (sa.size() != 4)
            throw Exception("Invalid IpV4 address");

        bool stat = isOnlyDecimal(sa[0]);
        stat      = stat && isOnlyDecimal(sa[1]);
        stat      = stat && isOnlyDecimal(sa[2]);
        stat      = stat && isOnlyDecimal(sa[3]);

        if (!stat)
            throw Exception("Invalid IpV4 address: ", inp);

        char buf[33]{};
        inet_pton(AF_INET, inp.c_str(), buf);
        return *(uint32_t*)&buf[0];
    }

    String Net::Utils::networkToAsciiIpV4(const uint32_t& inp)
    {
        in_addr addr4{};
        memset(&addr4, 0, sizeof(in_addr));

        addr4.s_addr = inp;
        char buf[32];

        inet_ntop(AF_INET, &addr4, buf, 32);
        return {buf};
    }

    uint32_t Net::Utils::networkToHostLong(const uint32_t& inp)
    {
        return ntohl(inp);
    }

    uint16_t Net::Utils::hostToNetworkShort(const uint16_t& inp)
    {
        return htons(inp);
    }

    uint16_t Net::Utils::networkToHostShort(
        const uint16_t& inp)
    {
        return ntohs(inp);
    }

    bool Net::Utils::getHostInfo(
        HostInfo&     inf,
        const String& name)
    {
        addrinfo* info;
        if (getaddrinfo(name.c_str(), nullptr, nullptr, &info) != 0)
            return false;

        const addrinfo* ptr = info;
        while (ptr != nullptr)
        {
            Host host;
            host.name = name;

            switch (ptr->ai_family)
            {
            default:
            case AF_UNSPEC:
                host.family = AddressFamilyUnknown;
                break;
            case AF_UNIX:
                host.family = AddressFamilyUnix;
                break;
            case AF_INET:
                host.family = AddressFamilyINet;
                break;
            }

            switch (ptr->ai_socktype)
            {
            default:
                host.type = SocketUnknown;
                break;
            case SOCK_STREAM:
                host.type = SocketStream;
                break;
            case SOCK_DGRAM:
                host.type = SocketDatagram;
                break;
            case SOCK_RAW:
                host.type = SocketRaw;
                break;
            }

            switch (ptr->ai_protocol)
            {
            default:
            case PF_UNSPEC:
                host.protocol = ProtocolUnknown;
                break;
            case IPPROTO_RAW:
                host.protocol = ProtocolIpRaw;
                break;
            case IPPROTO_TCP:
                host.protocol = ProtocolIpTcp;
                break;
            case IPPROTO_UDP:
                host.protocol = ProtocolIpUdp;
                break;
            }

            if (host.family == AddressFamilyINet)
            {
                const sockaddr_in* addr = (const sockaddr_in*)ptr->ai_addr;

                host.address = networkToAsciiIpV4(addr->sin_addr.s_addr);
            }

            inf.push_back(host);
            ptr = ptr->ai_next;
        }

        freeaddrinfo(info);
        return true;
    }

    Host Net::Utils::iNetHost(const HostInfo& inf)
    {
        for (const auto& host : inf)
        {
            if (host.family == AddressFamilyINet)
                return host;
        }
        return {};
    }

    Json::Dictionary Net::Utils::toJson(const Host& host)
    {
        Json::Dictionary dict;
        dict.insert("name", host.name);
        dict.insert("address", host.address);
        dict.insert("family", toString(host.family));
        dict.insert("type", toString(host.type));
        dict.insert("protocol", toString(host.protocol));
        return dict;
    }

    void Net::Utils::toJson(const Json::MixedArray& arr, const HostInfo& info)
    {
        for (const auto& host : info)
            arr.push(toJson(host));
    }  // namespace Rt2::Sockets::Net

    void Net::Utils::toStream(OStream& dest, const HostInfo& info)
    {
        const Json::MixedArray arr;
        toJson(arr, info);
        Ts::println(dest, arr.formatted());
    }

    void Net::Utils::toStream(OStream& dest, const Host& info)
    {
        Ts::println(dest, toJson(info).formatted());
    }

    String Net::Utils::toString(const HostInfo& info)
    {
        const Json::MixedArray arr;
        toJson(arr, info);
        return arr.formatted();
    }

    String Net::Utils::toString(const Host& info)
    {
        return toJson(info).formatted();
    }

    HostEnumerator::HostEnumerator(const String& hostName) :
        _name(hostName)
    {
        Net::ensureInitialized();
        Net::Utils::getHostInfo(_hosts, hostName);
    }

    const HostInfo& HostEnumerator::hosts()
    {
        return _hosts;
    }

    void HostEnumerator::log(OStream& out) const
    {
        Net::Utils::toStream(out, _hosts);
    }

    bool HostEnumerator::match(Host&               dest,
                               const AddressFamily family,
                               const Protocol      protocol,
                               const SocketType    type) const
    {
        for (const auto& host : _hosts)
        {
            if (host.address.empty()) continue;
            if (host.family != family) continue;

            if (protocol != ProtocolUnknown)
                if (host.protocol != protocol) continue;
            if (type != SocketUnknown)
                if (host.type != type) continue;

            dest = host;
            if (dest.type == SocketUnknown)
                dest.type = SocketStream;
            return true;
        }

        dest = {};
        return false;
    }

    String Net::Utils::toString(const AddressFamily& addressFamily)
    {
        switch (addressFamily)
        {
        case AddressFamilyUnknown:
            return "Unspecified";
        case AddressFamilyINet:
            return "INet";
        case AddressFamilyUnix:
            return "Unix";
        default:
            return "Unknown";
        }
    }

    String Net::Utils::toString(const Protocol& protocolFamily)
    {
        switch (protocolFamily)
        {
        case ProtocolUnknown:
            return "Unspecified";
        case ProtocolIpTcp:
            return "IpTCP";
        case ProtocolIpUdp:
            return "IpUDP";
        case ProtocolIpRaw:
            return "IpRaw";
        default:
            return "Unknown";
        }
    }

    String Net::Utils::toString(const SocketType& socketType)
    {
        switch (socketType)
        {
        case SocketDatagram:
            return "Datagram";
        case SocketStream:
            return "Stream";
        case SocketRaw:
            return "Raw";
        default:
        case SocketUnknown:
            return "Unknown";
        }
    }

    void Net::ensureInitialized()
    {
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
        class PlatformSetup
        {
        public:
            PlatformSetup()
            {
                WSADATA data = {};
                // The current version of the Windows Sockets specification is version 2.2.
                constexpr WORD versionRequested = MAKEWORD(2, 2);

                // If successful, the WSAStartup function returns zero
                if (const int status = WSAStartup(versionRequested, &data); status != 0)
                    Error::log();
            }

            ~PlatformSetup()
            {
                // If successful, the WSACleanup function returns zero.
                if (const int status = WSACleanup(); status != 0)
                    Error::log();
            }
        };
        static PlatformSetup inst;
#endif
    }

    void Net::Error::log()
    {
        log(std::cout);
    }

    void Net::Error::error()
    {
        OutputStringStream obs;
        log(obs);
        throw Exception(obs.str());
    }

    // Descriptions:
    // https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
    // https://man7.org/linux/man-pages/man3/errno.3.html
    void Net::Error::log(OStream& out)
    {
#if RT_PLATFORM == RT_PLATFORM_WINDOWS
        switch (WSAGetLastError())
        {
        case WSA_INVALID_HANDLE:
            Ts::println(out, "Specified event object handle is invalid.");
            break;
        case WSA_NOT_ENOUGH_MEMORY:
            Ts::println(out, "Insufficient memory available.");
            break;
        case WSA_INVALID_PARAMETER:
            Ts::println(out, "One or more parameters are invalid.");
            break;
        case WSA_OPERATION_ABORTED:
            Ts::println(out, "Overlapped operation aborted.");
            break;
        case WSA_IO_INCOMPLETE:
            Ts::println(out, "Overlapped I/O event object not in signaled state.");
            break;
        case WSA_IO_PENDING:
            Ts::println(out, "Overlapped operations will complete later.");
            break;
        case WSAEINTR:
            Ts::println(out, "Interrupted function call.");
            break;
        case WSAEBADF:
            Ts::println(out, "File handle is not valid.");
            break;
        case WSAEACCES:
            Ts::println(out, "Permission denied.");
            break;
        case WSAEFAULT:
            Ts::println(out, "Bad address.");
            break;
        case WSAEINVAL:
            Ts::println(out, "Invalid argument.");
            break;
        case WSAEMFILE:
            Ts::println(out, "Too many open files.");
            break;
        case WSAEWOULDBLOCK:
            Ts::println(out, "Resource temporarily unavailable.");
            break;
        case WSAEINPROGRESS:
            Ts::println(out, "Operation now in progress.");
            break;
        case WSAEALREADY:
            Ts::println(out, "Operation already in progress.");
            break;
        case WSAENOTSOCK:
            Ts::println(out, "Socket operation on non-socket.");
            break;
        case WSAEDESTADDRREQ:
            Ts::println(out, "Destination address required.");
            break;
        case WSAEMSGSIZE:
            Ts::println(out, "Message too long.");
            break;
        case WSAEPROTOTYPE:
            Ts::println(out, "Protocol wrong type for socket.");
            break;
        case WSAENOPROTOOPT:
            Ts::println(out, "Bad protocol option.");
            break;
        case WSAEPROTONOSUPPORT:
            Ts::println(out, "Protocol not supported.");
            break;
        case WSAESOCKTNOSUPPORT:
            Ts::println(out, "Socket type not supported.");
            break;
        case WSAEOPNOTSUPP:
            Ts::println(out, "Operation not supported.");
            break;
        case WSAEPFNOSUPPORT:
            Ts::println(out, "Protocol family not supported.");
            break;
        case WSAEAFNOSUPPORT:
            Ts::println(out, "Address family not supported by protocol family.");
            break;
        case WSAEADDRINUSE:
            Ts::println(out, "Address already in use.");
            break;
        case WSAEADDRNOTAVAIL:
            Ts::println(out, "Cannot assign requested address.");
            break;
        case WSAENETDOWN:
            Ts::println(out, "Network is down.");
            break;
        case WSAENETUNREACH:
            Ts::println(out, "Network is unreachable.");
            break;
        case WSAENETRESET:
            Ts::println(out, "Network dropped connection on reset.");
            break;
        case WSAECONNABORTED:
            Ts::println(out, "Software caused connection abort.");
            break;
        case WSAECONNRESET:
            Ts::println(out, "Connection reset by peer.");
            break;
        case WSAENOBUFS:
            Ts::println(out, "No buffer space available.");
            break;
        case WSAEISCONN:
            Ts::println(out, "Socket is already connected.");
            break;
        case WSAENOTCONN:
            Ts::println(out, "Socket is not connected.");
            break;
        case WSAESHUTDOWN:
            Ts::println(out, "Cannot send after socket shutdown.");
            break;
        case WSAETOOMANYREFS:
            Ts::println(out, "Too many references.");
            break;
        case WSAETIMEDOUT:
            Ts::println(out, "Connection timed out.");
            break;
        case WSAECONNREFUSED:
            Ts::println(out, "Connection refused.");
            break;
        case WSAELOOP:
            Ts::println(out, "Cannot translate name.");
            break;
        case WSAENAMETOOLONG:
            Ts::println(out, "Name too long.");
            break;
        case WSAEHOSTDOWN:
            Ts::println(out, "Host is down.");
            break;
        case WSAEHOSTUNREACH:
            Ts::println(out, "No route to host.");
            break;
        case WSAENOTEMPTY:
            Ts::println(out, "Directory not empty.");
            break;
        case WSAEPROCLIM:
            Ts::println(out, "Too many processes.");
            break;
        case WSAEUSERS:
            Ts::println(out, "User quota exceeded.");
            break;
        case WSAEDQUOT:
            Ts::println(out, "Disk quota exceeded.");
            break;
        case WSAESTALE:
            Ts::println(out, "Stale file handle reference.");
            break;
        case WSAEREMOTE:
            Ts::println(out, "Item is remote.");
            break;
        case WSASYSNOTREADY:
            Ts::println(out, "Network subsystem is unavailable.");
            break;
        case WSAVERNOTSUPPORTED:
            Ts::println(out, "Winsock.dll version out of range.");
            break;
        case WSANOTINITIALISED:
            Ts::println(out, "Successful WSAStartup not yet performed.");
            break;
        case WSAEDISCON:
            Ts::println(out, "Graceful shutdown in progress.");
            break;
        case WSAENOMORE:
            Ts::println(out, "No more results.");
            break;
        case WSAECANCELLED:
            Ts::println(out, "Call has been canceled.");
            break;
        case WSAEINVALIDPROCTABLE:
            Ts::println(out, "Procedure call table is invalid.");
            break;
        case WSAEINVALIDPROVIDER:
            Ts::println(out, "Service provider is invalid.");
            break;
        case WSAEPROVIDERFAILEDINIT:
            Ts::println(out, "Service provider failed to initialize.");
            break;
        case WSASYSCALLFAILURE:
            Ts::println(out, "System call failure.");
            break;
        case WSASERVICE_NOT_FOUND:
            Ts::println(out, "Service not found.");
            break;
        case WSATYPE_NOT_FOUND:
            Ts::println(out, "Class type not found.");
            break;
        case WSA_E_NO_MORE:
            Ts::println(out, "No more results.");
            break;
        case WSA_E_CANCELLED:
            Ts::println(out, "Call was canceled.");
            break;
        case WSAEREFUSED:
            Ts::println(out, "Database query was refused.");
            break;
        case WSAHOST_NOT_FOUND:
            Ts::println(out, "Host not found.");
            break;
        case WSATRY_AGAIN:
            Ts::println(out, "Non authoritative host not found.");
            break;
        case WSANO_RECOVERY:
            Ts::println(out, "This is a nonrecoverable error.");
            break;
        case WSANO_DATA:
            Ts::println(out, "Valid name, no data record of requested type.");
            break;
        case WSA_QOS_RECEIVERS:
            Ts::println(out, "QoS receivers.");
            break;
        case WSA_QOS_SENDERS:
            Ts::println(out, "QoS senders.");
            break;
        case WSA_QOS_NO_SENDERS:
            Ts::println(out, "No QoS senders.");
            break;
        case WSA_QOS_NO_RECEIVERS:
            Ts::println(out, "QoS no receivers.");
            break;
        case WSA_QOS_REQUEST_CONFIRMED:
            Ts::println(out, "QoS request confirmed.");
            break;
        case WSA_QOS_ADMISSION_FAILURE:
            Ts::println(out, "QoS admission error.");
            break;
        case WSA_QOS_POLICY_FAILURE:
            Ts::println(out, "QoS policy failure.");
            break;
        case WSA_QOS_BAD_STYLE:
            Ts::println(out, "QoS bad style.");
            break;
        case WSA_QOS_BAD_OBJECT:
            Ts::println(out, "QoS bad object.");
            break;
        case WSA_QOS_TRAFFIC_CTRL_ERROR:
            Ts::println(out, "QoS traffic control error.");
            break;
        case WSA_QOS_GENERIC_ERROR:
            Ts::println(out, "QoS generic error.");
            break;
        case WSA_QOS_ESERVICETYPE:
            Ts::println(out, "QoS service type error.");
            break;
        case WSA_QOS_EFLOWSPEC:
            Ts::println(out, "QoS flowspec error.");
            break;
        case WSA_QOS_EPROVSPECBUF:
            Ts::println(out, "Invalid QoS provider buffer.");
            break;
        case WSA_QOS_EFILTERSTYLE:
            Ts::println(out, "Invalid QoS filter style.");
            break;
        case WSA_QOS_EFILTERTYPE:
            Ts::println(out, "Invalid QoS filter type.");
            break;
        case WSA_QOS_EFILTERCOUNT:
            Ts::println(out, "Incorrect QoS filter count.");
            break;
        case WSA_QOS_EOBJLENGTH:
            Ts::println(out, "Invalid QoS object length.");
            break;
        case WSA_QOS_EFLOWCOUNT:
            Ts::println(out, "Incorrect QoS flow count.");
            break;
        case WSA_QOS_EUNKOWNPSOBJ:
            Ts::println(out, "Unrecognized QoS object.");
            break;
        case WSA_QOS_EPOLICYOBJ:
            Ts::println(out, "Invalid QoS policy object.");
            break;
        case WSA_QOS_EFLOWDESC:
            Ts::println(out, "Invalid QoS flow descriptor.");
            break;
        case WSA_QOS_EPSFLOWSPEC:
            Ts::println(out, "Invalid QoS provider-specific flowspec.");
            break;
        case WSA_QOS_EPSFILTERSPEC:
            Ts::println(out, "Invalid QoS provider-specific filterspec.");
            break;
        case WSA_QOS_ESDMODEOBJ:
            Ts::println(out, "Invalid QoS shape discard mode object.");
            break;
        case WSA_QOS_ESHAPERATEOBJ:
            Ts::println(out, "Invalid QoS shaping rate object.");
            break;
        case WSA_QOS_RESERVED_PETYPE:
            Ts::println(out, "Reserved policy QoS element type.");
            break;
        default:
            Ts::println(out, "unknown error");
            break;
        }
#else
        switch (errno)
        {
        case E2BIG:
            Ts::println(out, "Argument list too long.");
            break;
        case EACCES:
            Ts::println(out, "Permission denied.");
            break;
        case EADDRINUSE:
            Ts::println(out, "Address already in use.");
            break;
        case EADDRNOTAVAIL:
            Ts::println(out, "Address not available.");
            break;
        case EAFNOSUPPORT:
            Ts::println(out, "Address family not supported.");
            break;
        case EAGAIN:
            Ts::println(out, "Resource temporarily unavailable.");
            break;
        case EALREADY:
            Ts::println(out, "Connection already in progress.");
            break;
        case EBADE:
            Ts::println(out, "Invalid exchange.");
            break;
        case EBADF:
            Ts::println(out, "Bad file descriptor.");
            break;
        case EBADFD:
            Ts::println(out, "File descriptor in bad state.");
            break;
        case EBADMSG:
            Ts::println(out, "Bad message.");
            break;
        case EBADR:
            Ts::println(out, "Invalid request descriptor.");
            break;
        case EBADRQC:
            Ts::println(out, "Invalid request code.");
            break;
        case EBADSLT:
            Ts::println(out, "Invalid slot.");
            break;
        case EBUSY:
            Ts::println(out, "Device or resource busy.");
            break;
        case ECANCELED:
            Ts::println(out, "Operation canceled.");
            break;
        case ECHILD:
            Ts::println(out, "No child processes.");
            break;
        case ECHRNG:
            Ts::println(out, "Channel number out of range.");
            break;
        case ECOMM:
            Ts::println(out, "Communication error on send.");
            break;
        case ECONNABORTED:
            Ts::println(out, "Connection aborted.");
            break;
        case ECONNREFUSED:
            Ts::println(out, "Connection refused.");
            break;
        case ECONNRESET:
            Ts::println(out, "Connection reset.");
            break;
        case EDEADLK:
            Ts::println(out, "Resource deadlock avoided.");
            break;
        // case EDEADLOCK:
        //     Ts::println(out, "Resource deadlock avoided.");
        //     break;
        case EDESTADDRREQ:
            Ts::println(out, "Destination address required.");
            break;
        case EDOM:
            Ts::println(out, "Mathematics argument out of domain of function.");
            break;
        case EDQUOT:
            Ts::println(out, "Disk quota exceeded.");
            break;
        case EEXIST:
            Ts::println(out, "File exists.");
            break;
        case EFAULT:
            Ts::println(out, "Bad address.");
            break;
        case EFBIG:
            Ts::println(out, "File too large.");
            break;
        case EHOSTDOWN:
            Ts::println(out, "Host is down.");
            break;
        case EHOSTUNREACH:
            Ts::println(out, "Host is unreachable.");
            break;
        case EHWPOISON:
            Ts::println(out, "Memory page has hardware error.");
            break;
        case EIDRM:
            Ts::println(out, "Identifier removed.");
            break;
        case EILSEQ:
            Ts::println(out, "Invalid or incomplete multibyte or wide character.");
            break;
        case EINPROGRESS:
            Ts::println(out, "Operation in progress.");
            break;
        case EINTR:
            Ts::println(out, "Interrupted function call.");
            break;
        case EINVAL:
            Ts::println(out, "Invalid argument.");
            break;
        case EIO:
            Ts::println(out, "Input/output error.");
            break;
        case EISCONN:
            Ts::println(out, "Socket is connected.");
            break;
        case EISDIR:
            Ts::println(out, "Is a directory.");
            break;
        case EISNAM:
            Ts::println(out, "Is a named type file.");
            break;
        case EKEYEXPIRED:
            Ts::println(out, "Key has expired.");
            break;
        case EKEYREJECTED:
            Ts::println(out, "Key was rejected by service.");
            break;
        case EKEYREVOKED:
            Ts::println(out, "Key has been revoked.");
            break;
        case EL2HLT:
            Ts::println(out, "Level 2 halted.");
            break;
        case EL2NSYNC:
            Ts::println(out, "Level 2 not synchronized.");
            break;
        case EL3HLT:
            Ts::println(out, "Level 3 halted.");
            break;
        case EL3RST:
            Ts::println(out, "Level 3 reset.");
            break;
        case ELIBACC:
            Ts::println(out, "Cannot access a needed shared library.");
            break;
        case ELIBBAD:
            Ts::println(out, "Accessing a corrupted shared library.");
            break;
        case ELIBMAX:
            Ts::println(out, "Attempting to link in too many shared libraries.");
            break;
        case ELIBSCN:
            Ts::println(out, ".lib section in a.out corrupted");
            break;
        case ELIBEXEC:
            Ts::println(out, "Cannot exec a shared library directly.");
            break;
        case ELNRNG:
            Ts::println(out, "Link number out of range.");
            break;
        case ELOOP:
            Ts::println(out, "Too many levels of symbolic links.");
            break;
        case EMEDIUMTYPE:
            Ts::println(out, "Wrong medium type.");
            break;
        case EMFILE:
            Ts::println(out, "Too many open files.");
            break;
        case EMLINK:
            Ts::println(out, "Too many links.");
            break;
        case EMSGSIZE:
            Ts::println(out, "Message too long.");
            break;
        case EMULTIHOP:
            Ts::println(out, "Multihop attempted.");
            break;
        case ENAMETOOLONG:
            Ts::println(out, "Filename too long.");
            break;
        case ENETDOWN:
            Ts::println(out, "Network is down.");
            break;
        case ENETRESET:
            Ts::println(out, "Connection aborted by network.");
            break;
        case ENETUNREACH:
            Ts::println(out, "Network unreachable.");
            break;
        case ENFILE:
            Ts::println(out, "Too many open files in system.");
            break;
        case ENOANO:
            Ts::println(out, "No anode.");
            break;
        case ENOBUFS:
            Ts::println(out, "No buffer space available.");
            break;
        case ENODATA:
            Ts::println(out, "The named attribute does not exist, or the process has no access to this attribute");
            break;
        case ENODEV:
            Ts::println(out, "No such device.");
            break;
        case ENOENT:
            Ts::println(out, "No such file or directory.");
            break;
        case ENOEXEC:
            Ts::println(out, "Exec format error.");
            break;
        case ENOKEY:
            Ts::println(out, "Required key not available.");
            break;
        case ENOLCK:
            Ts::println(out, "No locks available.");
            break;
        case ENOLINK:
            Ts::println(out, "Link has been severed.");
            break;
        case ENOMEDIUM:
            Ts::println(out, "No medium found.");
            break;
        case ENOMEM:
            Ts::println(out, "Not enough space/cannot allocate memory.");
            break;
        case ENOMSG:
            Ts::println(out, "No message of the desired type.");
            break;
        case ENONET:
            Ts::println(out, "Machine is not on the network.");
            break;
        case ENOPKG:
            Ts::println(out, "Package not installed.");
            break;
        case ENOPROTOOPT:
            Ts::println(out, "Protocol not available.");
            break;
        case ENOSPC:
            Ts::println(out, "No space left on device.");
            break;
        case ENOSR:
            Ts::println(out, "No STREAM resources.");
            break;
        case ENOSTR:
            Ts::println(out, "Not a STREAM.");
            break;
        case ENOSYS:
            Ts::println(out, "Function not implemented.");
            break;
        case ENOTBLK:
            Ts::println(out, "Block device required.");
            break;
        case ENOTCONN:
            Ts::println(out, "The socket is not connected.");
            break;
        case ENOTDIR:
            Ts::println(out, "Not a directory.");
            break;
        case ENOTEMPTY:
            Ts::println(out, "Directory not empty.");
            break;
        case ENOTRECOVERABLE:
            Ts::println(out, "State not recoverable.");
            break;
        case ENOTSOCK:
            Ts::println(out, "Not a socket.");
            break;
        // case ENOTSUP:
        //     Ts::println(out, "Operation not supported.");
        //     break;
        case EOPNOTSUPP:
            Ts::println(out, "Operation not supported on socket.");
            break;
        case ENOTTY:
            Ts::println(out, "Inappropriate I/O control operation.");
            break;
        case ENOTUNIQ:
            Ts::println(out, "Name not unique on network.");
            break;
        case ENXIO:
            Ts::println(out, "No such device or address.");
            break;
        case EOVERFLOW:
            Ts::println(out, "Value too large to be stored in data type.");
            break;
        case EOWNERDEAD:
            Ts::println(out, "Owner died.");
            break;
        case EPERM:
            Ts::println(out, "Operation not permitted.");
            break;
        case EPFNOSUPPORT:
            Ts::println(out, "Protocol family not supported.");
            break;
        case EPIPE:
            Ts::println(out, "Broken pipe.");
            break;
        case EPROTO:
            Ts::println(out, "Protocol error.");
            break;
        case EPROTONOSUPPORT:
            Ts::println(out, "Protocol not supported.");
            break;
        case EPROTOTYPE:
            Ts::println(out, "Protocol wrong type for socket.");
            break;
        case ERANGE:
            Ts::println(out, "Result too large.");
            break;
        case EREMCHG:
            Ts::println(out, "Remote address changed.");
            break;
        case EREMOTE:
            Ts::println(out, "Object is remote.");
            break;
        case EREMOTEIO:
            Ts::println(out, "Remote I/O error.");
            break;
        case ERESTART:
            Ts::println(out, "Interrupted system call should be restarted.");
            break;
        case ERFKILL:
            Ts::println(out, "Operation not possible due to RF-kill.");
            break;
        case EROFS:
            Ts::println(out, "Read-only filesystem.");
            break;
        case ESHUTDOWN:
            Ts::println(out, "Cannot send after transport endpoint shutdown.");
            break;
        case ESPIPE:
            Ts::println(out, "Invalid seek.");
            break;
        case ESOCKTNOSUPPORT:
            Ts::println(out, "Socket type not supported.");
            break;
        case ESRCH:
            Ts::println(out, "No such process.");
            break;
        case ESTALE:
            Ts::println(out, "Stale file handle.");
            break;
        case ESTRPIPE:
            Ts::println(out, "Streams pipe error.");
            break;
        case ETIME:
            Ts::println(out, "Timer expired.");
            break;
        case ETIMEDOUT:
            Ts::println(out, "Connection timed out.");
            break;
        case ETOOMANYREFS:
            Ts::println(out, "Too many references: cannot splice.");
            break;
        case ETXTBSY:
            Ts::println(out, "Text file busy.");
            break;
        case EUCLEAN:
            Ts::println(out, "Structure needs cleaning.");
            break;
        case EUNATCH:
            Ts::println(out, "Protocol driver not attached.");
            break;
        case EUSERS:
            Ts::println(out, "Too many users.");
            break;
        case EXDEV:
            Ts::println(out, "Invalid cross-device link.");
            break;
        case EXFULL:
            Ts::println(out, "Exchange full.");
            break;
        default:
            Ts::println(out, "Unknown error. (", errno, ')');
            break;
        }
#endif
    }
}  // namespace Rt2::Sockets
