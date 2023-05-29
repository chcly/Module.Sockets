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
#include <istream>
#include <ostream>
#include "Sockets/PlatformSocket.h"

namespace Rt2::Sockets
{
    class SocketOutputStream final : public std::ostream
    {
    public:
        class StreamBuffer final : public std::streambuf
        {
        private:
            Net::Socket _sock;

        public:
            explicit StreamBuffer(const Net::Socket& sock) :
                std::streambuf(),
                _sock(sock)
            {
            }

        protected:
            int_type overflow(const int_type ch) override  // put a character to stream (always fail)
            {
                if (ch > 0)
                    return Net::writeBuffer(_sock, &ch, 1);
                return traits_type::eof();
            }

            std::streamsize xsputn(const char* ptr, const std::streamsize count) override
            {
                if (ptr)
                    return Net::writeBuffer(_sock, ptr, count);
                return 0;
            }
        };

    private:
        StreamBuffer _buffer;

    public:
        explicit SocketOutputStream(const Net::Socket& socket) :
            std::ostream(&_buffer),
            _buffer(socket)
        {
        }
    };

    class SocketStream
    {
    private:
        Net::Socket _sock;
        Net::Status _status;

    public:
        explicit SocketStream(Net::Socket sock);
        ~SocketStream() = default;

        size_t read(void* destination, const size_t& length);

        bool eof() const;
    };

    inline bool SocketStream::eof() const
    {
        return _status != Net::Ok;
    }

    template <size_t ScratchSize = 0x7FFF>
    class SocketInputStream final : public std::istream
    {
    public:
        class StreamBuffer final : public std::streambuf
        {
        private:
            Net::Socket  _sock;
            char*        _scratch;
            char*        _begin;
            char*        _end;
            const size_t _max;
            Net::Status  _status;

            int_type more()
            {
                if (_status == Net::Done)
                    return traits_type::eof();

                int br  = 0;
                _status = Net::readBuffer(_sock,
                                          _scratch,
                                          (int)_max,
                                          br);

                const int_type bytesRead = br;

                _scratch[bytesRead] = 0;

                if (_status != Net::Error)
                {
                    _begin = _scratch;
                    _end   = _scratch + bytesRead;
                }
                return bytesRead;
            }

        public:
            explicit StreamBuffer(const Net::Socket& sock) :
                std::streambuf(),
                _sock(sock),
                _begin(nullptr),
                _end(nullptr),
                _max(ScratchSize),
                _status(Net::Ok)
            {
                _scratch = new char[_max + 1];
                more();
            }

            ~StreamBuffer() override
            {
                _begin = _end = nullptr;
                delete[] _scratch;
                _scratch = nullptr;
            }

        protected:
            std::streamsize showmanyc() override
            {
                return _end - _begin;
            }

            // Translation: get a character from stream, but don't point past it
            int_type underflow() override
            {
                if (_status == Net::Error)
                    return traits_type::eof();

                if (_begin == _end)
                {
                    if (_status == Net::Done)
                        return traits_type::eof();
                    more();
                }
                if (_status == Net::Error || !_begin)
                    return traits_type::eof();
                return traits_type::to_int_type(*_begin);
            }

            // Translation: get a character and point past it.
            int_type uflow() override
            {
                if (_status == Net::Error)
                    return traits_type::eof();

                if (_begin == _end)
                {
                    if (_status == Net::Done)
                        return traits_type::eof();
                    more();
                }

                if (_status == Net::Error || !_begin)
                    return traits_type::eof();

                return traits_type::to_int_type(*_begin++);
            }
        };

    private:
        StreamBuffer _buffer;

    public:
        explicit SocketInputStream(const Net::Socket& socket) :
            std::istream(&_buffer),
            _buffer(socket)
        {
        }
    };

    using DefaultInputStream = SocketInputStream<0x7FFF>;

}  // namespace Rt2::Sockets
