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
#define _CRT_SECURE_NO_WARNINGS 1
#include "PlatformSocket.h"
#include <cstdio>
#include <iostream>
#include "Connection.h"
#include "Utils/Char.h"
#include "Utils/Exception.h"

#ifndef _WIN32
#include <poll.h>
#include <sys/select.h>
#include <unistd.h>
#include <cstring>
#endif

namespace Rt2::Sockets::Net
{
    class Connection;

    void ensureInitialized()
    {
        class Setup
        {
        public:
            Setup()
            {
                try
                {
                    initialize();
                }
                catch (Exception& ex)
                {
                    Console::writeLine(ex.what());
                }
            }
            ~Setup()
            {
                Console::writeLine("finished");
                finalize();
            }
        };

        static Setup inst;
    }



    void initialize()
    {
#ifdef _WIN32
        WSADATA data = {};

        // The current version of the Windows Sockets specification is version 2.2.
        constexpr WORD versionRequested = MAKEWORD(2, 2);

        // If successful, the WSAStartup function returns zero
        if (const int status = WSAStartup(versionRequested, &data);
            status == 0)
        {
#ifdef VERBOSE_DEBUG
            printf("WSADATA   \n");
            printf(" Version     : %d.%d\n", LOBYTE(data.wVersion), HIBYTE(data.wVersion));
            printf(" MaxSockets  : %d\n", data.iMaxSockets);
            printf(" MaxDatagram : %d\n", data.iMaxUdpDg);
            printf(" Vendor      : %s\n", data.lpVendorInfo);
            printf(" Description : %s\n", data.szDescription);
            printf(" Status      : %s\n", data.szSystemStatus);
#endif
        }
        else
            throw Exception("Initialize Windows sockets failed with error code: ", WSAGetLastError());
#endif
    }

    void finalize()
    {
#ifdef _WIN32

        // If successful, the WSACleanup function returns zero.
        if (const int status = WSACleanup(); status != 0)
            throw Exception("Shutdown Windows sockets failed with error code: ", WSAGetLastError());
#endif
    }

    Socket create(const AddressFamily  addressFamily,
                  const SocketType     type,
                  const ProtocolFamily protocolFamily)
    {
        return socket((int)addressFamily, (int)type, (int)protocolFamily);
    }

    void close(const Socket& sock)
    {
#ifdef _WIN32
        closesocket(sock);
#else
        ::close(sock);
#endif
    }

    String toString(const AddressFamily& addressFamily)
    {
        switch (addressFamily)
        {
        case AddrUnspecified:
            return "AF_UNSPEC";
        case AddrINet:
            return "AF_INET";
        case AddrUnix:
            return "AF_UNIX";
        default:
            return "Other";
        }
    }

    String toString(const ProtocolFamily& protocolFamily)
    {
        switch (protocolFamily)
        {
        case ProtoUnspecified:
            return "PF_UNSPEC";
        case ProtoIpTcp:
            return "IPPROTO_TCP";
        case ProtoIpUdp:
            return "IPPROTO_UDP";
        case ProtoIpRaw:
            return "IPPROTO_RAW";
        default:
            return "Other";
        }
    }

    String toString(const SocketType& socketType)
    {
        switch (socketType)
        {
        case Datagram:
            return "SOCK_DGRAM";
        case Stream:
            return "SOCK_STREAM";
        case Raw:
            return "SOCK_RAW";
        case Rdm:
            return "SOCK_RDM";
        case Seq:
            return "SOCK_SEQPACKET";
        case UnknownSocket:
            return "UnknownSocket";
        default:
            return "Other";
        }
    }


    Status setOption(Socket&             sock,
                     const ProtocolLevel protocolLevel,
                     const SocketOption  optionType,
                     const void*         option,
                     const size_t        optionSize)
    {
#ifdef WIN32
        return (Status)setsockopt(sock,
                                  protocolLevel,
                                  (int)optionType,
                                  (const char*)option,
                                  (int)optionSize);
#else
        return (Status)setsockopt(sock,
                                  protocolLevel,
                                  (int)optionType,
                                  (const void*)option,
                                  (socklen_t)optionSize);
#endif
    }

    void constructInputAddress(SocketInputAddress& dest,
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
        dest.sin_port        = HostToNetworkShort(port);
        dest.sin_addr.s_addr = AsciiToNetworkIpV4(address);
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
        return true;
    }

    uint32_t AsciiToNetworkIpV4(const String& inp)
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
            throw Exception("Invalid IpV4 address");

        char buf[33]{};
        inet_pton(AF_INET, inp.c_str(), buf);
        return *(uint32_t*)&buf[0];
    }

    String NetworkToAsciiIpV4(const uint32_t& inp)
    {
        in_addr addr4{};
        memset(&addr4, 0, sizeof(in_addr));

        addr4.s_addr = inp;
        char buf[32];

        inet_ntop(AF_INET, &addr4, buf, 32);
        return {buf};
    }

    Status connect(const Socket& sock, const String& str, const uint16_t port)
    {
        SocketInputAddress inp = {};

        inp.sin_family      = AF_INET;
        inp.sin_port        = HostToNetworkShort(port);
        inp.sin_addr.s_addr = AsciiToNetworkIpV4(str);

        return (Status)::connect(sock, (const sockaddr*)&inp, sizeof(sockaddr));
    }

    uint32_t NetworkToHostLong(const uint32_t& inp)
    {
        return ntohl(inp);
    }

    uint16_t HostToNetworkShort(const uint16_t& inp)
    {
        return htons(inp);
    }

    uint16_t NetworkToHostShort(const uint16_t& inp)
    {
        return ntohs(inp);
    }

    Status bind(const Socket& sock, SocketInputAddress& addr)
    {
        return (Status)::bind(sock, (sockaddr*)&addr, sizeof(sockaddr));
    }

    Status listen(const Socket& sock, const int32_t backlog)
    {
        return (Status)::listen(sock, backlog);
    }

    Socket accept(const Socket& sock)
    {
        SocketInputAddress unused = {};

        int sz = sizeof(SocketInputAddress);
#ifdef _WIN32
        return ::accept(sock, (sockaddr*)&unused, &sz);
#else
        return ::accept(sock, (sockaddr*)&unused, (socklen_t*)&sz);
#endif
    }

    Socket accept(const Socket& sock, SocketInputAddress& dest)
    {
        dest = {};

        int sz = sizeof(SocketInputAddress);
#ifdef _WIN32
        return ::accept(sock, (sockaddr*)&dest, &sz);
#else
        return ::accept(sock, (sockaddr*)&dest, (socklen_t*)&sz);
#endif
    }

    Socket accept(const Socket& sock, Connection& result)
    {
        SocketInputAddress& dest = result.getInput();
        memset(&dest, 0, sizeof(SocketInputAddress));

        int sz = sizeof(SocketInputAddress);
#ifdef _WIN32
        return ::accept(sock, (sockaddr*)&dest, &sz);
#else
        return ::accept(sock, (sockaddr*)&dest, (socklen_t*)&sz);
#endif
    }

    Status readBuffer(const Socket& sock,
                      char*         buffer,
                      const int     bufLen,
                      int&          bytesRead)
    {
        if (!buffer)
            throw Exception("Invalid buffer supplied ");

        if (bufLen > MaxBufferSize)
            throw Exception("Invalid buffer size");

        bytesRead = 0;  // init to zero

        if (const int rl = recv(sock, buffer, bufLen, 0);
            rl > 0 && rl <= bufLen)
        {
            bytesRead  = rl;
            buffer[rl] = 0;

            // There is a bug lurking here,
            // when it equals the bufLen and there is no
            // more data to read. (another call will block)
            // which is not the desired behavior. AKA bug...
            if (rl < bufLen)
                return Done;
            return Ok;
        }
        return Error;
    }

    int writeBuffer(const Socket& sock,
                    const void*   buffer,
                    const size_t  bufLen)
    {
        if (!buffer)
            throw Exception("Invalid buffer supplied ");

        if (bufLen > MaxBufferSize)
            throw Exception("Invalid buffer size");

        return send(sock, (const char*)buffer, (int)bufLen, 0);
    }

    int writeFile(const Socket& sock, const char* fileName)
    {
        if (!fileName)
        {
            printf("invalid file name\n");
            return -1;
        }

        FILE* fp = fopen(fileName, "rb");
        if (!fp)
        {
            printf("Failed to open file: %s", fileName);
            return -1;
        }

        fseek(fp, 0L, SEEK_END);
        int bufLen = (int)ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        if (bufLen <= 0)
        {
            printf("File is empty");
            fclose(fp);
            return -1;
        }

        if (bufLen > 0xFFFF)
        {
            printf("File length is restricted to 0xFFFF\n");
            fclose(fp);
            return -1;
        }

        char* mem = (char*)malloc(bufLen + 2);
        if (mem == nullptr)
        {
            printf("Invalid alloc\n");
            fclose(fp);
            return -1;
        }

        fread(mem, 1, bufLen, fp);
        mem[bufLen] = 0;
        fclose(fp);

        bufLen = writeBuffer(sock, mem, bufLen);
        free(mem);
        return (int)bufLen;
    }

    void printHexLine(OStream&     dest,
                      const char*  buffer,
                      const size_t offs,
                      const size_t max)
    {
        size_t i, j;

        unsigned char ch;
        for (i = 0; i < 16; ++i)
        {
            j = i + offs;
            if (i % 9 == 8)
                dest << ' ';
            if (j >= max)
                dest << "00 ";
            else
            {
                ch = buffer[j];
                dest << std::setfill('0')
                     << std::setw(2)
                     << std::hex << (unsigned int)ch
                     << ' ';
            }
        }

        dest << '|';
        for (i = 0; i < 16; ++i)
        {
            j = i + offs;
            if (j >= max)
                dest << '.';
            else
            {
                ch = buffer[j];
                if (ch >= 32 && ch < 127)
                    dest << ch;
                else
                    dest << '.';
            }
        }

        dest << '|';
        dest << '\n';
    }

    void printHex(const char* buffer, const uint32_t len)
    {
        printHex(std::cout, buffer, len);
    }

    void printHex(OStream& dest, const char* buffer, const uint32_t len)
    {
        if (!buffer)
            throw Exception("Invalid buffer supplied ");

        for (size_t i = 0; i < len; i += 16)
            printHexLine(dest, buffer, i, len);
    }

    void printBuffer(const char* buffer, const uint32_t len)
    {
        if (!buffer)
            throw Exception("Invalid buffer supplied ");

        for (size_t i = 0; i < len; i++)
        {
            if (const char ch = buffer[i];
                ch == '\n' || ch >= 32 && ch < 127)
                putchar(ch);
        }
    }

    bool getHostInfo(HostInfo& inf, const String& name)
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
                host.family = AddrUnspecified;
                break;
            case AF_UNIX:
                host.family = AddrUnix;
                break;
            case AF_INET:
                host.family = AddrINet;
                break;
            }

            switch (ptr->ai_socktype)
            {
            default:
                host.type = UnknownSocket;
                break;
            case SOCK_STREAM:
                host.type = Stream;
                break;
            case SOCK_DGRAM:
                host.type = Datagram;
                break;
            case SOCK_RAW:
                host.type = Raw;
                break;
            }

            switch (ptr->ai_protocol)
            {
            default:
            case PF_UNSPEC:
                host.protocol = ProtoUnspecified;
                break;
            case IPPROTO_RAW:
                host.protocol = ProtoIpRaw;
                break;
            case IPPROTO_TCP:
                host.protocol = ProtoIpTcp;
                break;
            case IPPROTO_UDP:
                host.protocol = ProtoIpUdp;
                break;
            }

            if (host.family == AddrINet)
            {
                const sockaddr_in* addr = (const sockaddr_in*)ptr->ai_addr;

                host.address = NetworkToAsciiIpV4(addr->sin_addr.s_addr);
            }

            inf.push_back(host);
            ptr = ptr->ai_next;
        }

        freeaddrinfo(info);
        return true;
    }

    String toString(const HostInfo& info)
    {
        OutputStringStream oss;
        int                i = 0;
        for (const auto& [name, address, family, type, protocol] : info)
        {
            oss << "Host[" << i++ << "] {" << std::endl;
            oss << "    name     : " << name << std::endl;
            oss << "    address  : " << address << std::endl;
            oss << "    family   : " << toString(family) << std::endl;
            oss << "    type     : " << toString(type) << std::endl;
            oss << "    protocol : " << toString(protocol) << std::endl;
            oss << "}" << std::endl;
        }
        return oss.str();
    }

}  // namespace Rt2::Sockets::Plat
