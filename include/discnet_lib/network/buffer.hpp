/*
 *
 */

#pragma once

#include <memory>
#include <span>
#include <discnet_lib/discnet_lib.hpp>
#include <discnet_lib/typedefs.hpp>
#include <boost/asio/buffer.hpp>

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

        DISCNET_EXPORT const_buffer_t const_buffer() const;

        template <typename type_t>
        DISCNET_EXPORT type_t read();
        template <typename type_t>
        DISCNET_EXPORT bool append(type_t val);

        DISCNET_EXPORT buffer_span_t read_buffer(size_t length);
        DISCNET_EXPORT bool append(const buffer_span_t& buffer);

        DISCNET_EXPORT void reset_write();
        DISCNET_EXPORT void reset_read();
        DISCNET_EXPORT void resize(size_t new_size);
    private:
        size_t m_write_offset;
        size_t m_read_offset;
        std::vector<discnet::byte_t> m_buffer;
    };
} // ! namespace discnet::network