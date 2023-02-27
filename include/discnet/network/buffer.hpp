/*
 *
 */

#pragma once

#include <memory>
#include <span>
#include <discnet/discnet.hpp>
#include <discnet/typedefs.hpp>
#include <boost/asio/buffer.hpp>

// C:\Users\smashcomp\Desktop\void\projects\discnet\build\discnet_lib\Debug>dumpbin /ARCHIVEMEMBERS discnet_lib.lib

namespace discnet::network
{
    typedef std::span<const discnet::byte_t> buffer_span_t;
    typedef boost::asio::const_buffer const_buffer_t;

    class buffer_t
    {
    public:
        DISCNET_EXPORT buffer_t(size_t size);

        DISCNET_EXPORT buffer_span_t data() const;
        DISCNET_EXPORT size_t remaining_bytes() const;
        DISCNET_EXPORT size_t appended_bytes() const;
        DISCNET_EXPORT size_t bytes_left_to_read() const;
        DISCNET_EXPORT size_t size() const;

        DISCNET_EXPORT const_buffer_t const_buffer() const;

        DISCNET_EXPORT uint16_t read_uint16();
        DISCNET_EXPORT uint32_t read_uint32();
        DISCNET_EXPORT uint64_t read_uint64();
        DISCNET_EXPORT int16_t read_int16();
        DISCNET_EXPORT int32_t read_int32();
        DISCNET_EXPORT int64_t read_int64();

        DISCNET_EXPORT bool append(uint16_t val);
        DISCNET_EXPORT bool append(uint32_t val);
        DISCNET_EXPORT bool append(uint64_t val);
        DISCNET_EXPORT bool append(int16_t val);
        DISCNET_EXPORT bool append(int32_t val);
        DISCNET_EXPORT bool append(int64_t val);

        DISCNET_EXPORT buffer_span_t read_buffer(size_t length);
        DISCNET_EXPORT bool append(const buffer_span_t& buffer);

        DISCNET_EXPORT void reset_write();
        DISCNET_EXPORT void reset_read();
        DISCNET_EXPORT void resize(size_t new_size);
    private:
        template <typename type_t> type_t read();
        template <typename type_t> bool append(type_t val);

        size_t m_write_offset;
        size_t m_read_offset;
        std::vector<discnet::byte_t> m_buffer;
    };
} // ! namespace discnet::network