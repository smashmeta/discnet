/*
 *
 */

#pragma once

#include <discnet/network/buffer.hpp>

namespace discnet::network
{
    class read_buffer_t
    {
    public:
        DISCNET_EXPORT read_buffer_t(buffer_span_t buffer);

        DISCNET_EXPORT size_t bytes_left_to_read() const;
        DISCNET_EXPORT size_t size() const;

        DISCNET_EXPORT uint16_t read_uint16();
        DISCNET_EXPORT uint32_t read_uint32();
        DISCNET_EXPORT uint64_t read_uint64();
        DISCNET_EXPORT int16_t read_int16();
        DISCNET_EXPORT int32_t read_int32();
        DISCNET_EXPORT int64_t read_int64();

        DISCNET_EXPORT buffer_span_t read_buffer(size_t length);

        DISCNET_EXPORT void reset_read();
    private:
        template <typename type_t> type_t read();
        
        size_t m_read_offset;
        buffer_span_t m_buffer;
    };
} // ! namespace discnet::network