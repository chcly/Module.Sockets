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
#include "Utils/Streams/StreamBase.h"

namespace Rt2::Sockets
{
    namespace Default
    {
        constexpr size_t ScratchSize = IoBufferSize;
        constexpr int    TimeOut     = SocketTimeOut;
    }  // namespace Default


    class OutputSocketStream final : public std::ostream
    {
    public:
        class StreamBuffer final : public std::streambuf
        {
        private:
            PlatformSocket _sock;

        public:
            explicit StreamBuffer(const PlatformSocket& sock) :
                _sock(sock)
            {
            }

        protected:
            int_type overflow(const int_type ch) override
            {
                if (ch > 0) return Net::writeSocket(_sock, &ch, 1, Default::TimeOut);
                return traits_type::eof();
            }

            std::streamsize xsputn(const char* ptr, const std::streamsize count) override
            {
                if (ptr) return Net::writeSocket(_sock, ptr, count, Default::TimeOut);
                return traits_type::eof();
            }
        };

    private:
        StreamBuffer _buffer;

    public:
        explicit OutputSocketStream(const PlatformSocket& socket) :
            std::ostream(&_buffer),
            _buffer(socket)
        {
        }

        template <typename... Args>
        void println(Args&&... args)
        {
            OStream& os = *this;
            ((os << std::forward<Args>(args)), ...);
            os << std::endl;
        }

        template <typename... Args>
        void write(Args&&... args)
        {
            OStream& os = *this;
            ((os << std::forward<Args>(args)), ...);
        }
    };

    class InputSocketStream final : public std::istream
    {
    public:
        using BufferType       = Stream::BufferType;
        using PointerType      = BufferType::PointerType;
        using ConstPointerType = BufferType::ConstPointerType;

    public:
        class StreamBuffer final : public std::streambuf
        {
        private:
            PlatformSocket _sock{InvalidSocket};
            BufferType     _buffer;
            PointerType    _begin{nullptr};
            PointerType    _end{nullptr};
            std::streampos _total{0};
            Status         _status{OkStatus};
            int            _timeout{Default::TimeOut};
            int            _scratch{Default::ScratchSize};

            bool canRead() const
            {
                return _status == OkStatus;
            }

            int_type readMore()
            {
                if (!canRead())
                    return traits_type::eof();

                if (_scratch < 16)  // bare bone minimum to read
                    return traits_type::eof();

                int br = 0;
                _buffer.reserve(_buffer.size() + _scratch);

                _status = Net::readSocket(_sock,
                                          _buffer._end(),
                                          _scratch,
                                          br,
                                          _timeout);

                _buffer.resizeFast(_buffer.size() + br);
                if (br > 0)
                {
                    _begin = _buffer.begin() + _total;
                    _end   = _begin + br;
                    RT_ASSERT(_end <= _buffer.end())
                    _total += br;
                }
                else
                {
                    _begin = _end = nullptr;
                }
                return br;
            }

        public:
            explicit StreamBuffer(const PlatformSocket& sock) :
                _sock(sock)
            {
            }

            ~StreamBuffer() override = default;

            void string(String& str)
            {
                str = {
                    _buffer.data(),
                    _buffer.size(),
                };
            }

            Stream::BufferType& iBuf() { return _buffer; }

            void setTimeout(const int timeout)
            {
                _timeout = timeout;
            }

            void setBlockSize(const size_t block)
            {
                _scratch = Clamp<int>((int)block, Default::ScratchSize, 0x7FFF);
            }

        protected:
            std::streamsize showmanyc() override
            {
                return _end - _begin;
            }

            bool isFinished()
            {
                if (_begin == _end)
                {
                    if (!canRead())
                        return true;
                    readMore();
                }
                return false;
            }

            int_type underflow() override
            {
                if (isFinished())
                    return traits_type::eof();
                if (_begin)
                    return traits_type::to_int_type(*_begin);
                return traits_type::eof();
            }

            int_type uflow() override
            {
                if (isFinished())
                    return traits_type::eof();
                if (_begin && _begin + 1 <= _end)
                    return traits_type::to_int_type(*_begin++);
                return traits_type::eof();
            }

            int_type pbackfail(int_type) override
            {
                if (_total > 0 && _begin)
                {
                    _total -= 1;
                    --_begin;
                    return 1;
                }
                return traits_type::eof();
            }
        };

    private:
        StreamBuffer _buffer;

    public:
        explicit InputSocketStream(const PlatformSocket& socket) :
            std::istream(&_buffer),
            _buffer(socket)
        {
        }

        void string(String& str)
        {
            _buffer.string(str);
        }

        void setTimeout(const int timeout)
        {
            _buffer.setTimeout(timeout);
        }

        void setBlockSize(const size_t size)
        {
            _buffer.setBlockSize(size);
        }

        String string()
        {
            String copy;
            copyTo(copy);
            return copy;
        }

        void copyTo(OStream& in)
        {
            Stream::BufferType buf;
            readTo(buf);
            in.write(buf.data(), (std::streamsize)buf.size());
        }

        void copyTo(String& in)
        {
            Stream::BufferType buf;
            readTo(buf);
            in.assign(buf.data(), (std::streamsize)buf.size());
        }

        template <typename... Args>
        void get(Args&&... args)
        {
            IStream& os = *this;
            ((os >> std::forward<Args>(args)), ...);
        }

    private:

        void readTo(Stream::BufferType& buf)
        {
            buf.resizeFast(0);

            while (!eof())
            {
                buf.reserve(buf.size() + Default::ScratchSize);
                read(buf._end(), Default::ScratchSize);
                buf.resizeFast(buf.size() + gcount());
            }
        }
    };

}  // namespace Rt2::Sockets
