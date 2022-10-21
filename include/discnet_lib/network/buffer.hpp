/*
 *
 */

#pragma once

#include <memory>
#include <span>
#include <discnet_lib/discnet_lib.hpp>
#include <discnet_lib/typedefs.hpp>

namespace discnet::network
{
    typedef std::span<const discnet::byte_t> buffer_span_t;

    class buffer_t
    {
    public:
        buffer_t(const size_t size);

        buffer_span_t data() const;
        size_t remaining_bytes() const;
        size_t appended_bytes() const;

        bool append(const buffer_span_t& buffer);
        bool append(const uint8_t val);
        bool append(const int8_t val);
        bool append(const uint16_t val);
        bool append(const int16_t val);
        bool append(const uint32_t val);
        bool append(const int32_t val);
        bool append(const uint64_t val);
        bool append(const int64_t val);

        void reset();
        void resize(const size_t new_size);
    private:
        size_t m_offset;
        std::vector<discnet::byte_t> m_buffer;
    };
}