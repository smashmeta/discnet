/*
 *
 */

#include <discnet/network/read_buffer.hpp>


namespace discnet::network
{
    read_buffer_t::read_buffer_t(buffer_span_t buffer)
        : m_read_offset(0), m_buffer(buffer)
    {
        // nothing for now
    }

    size_t read_buffer_t::bytes_left_to_read() const
    {
        if (m_read_offset >= m_buffer.size())
        {
            return 0;
        }

        return m_buffer.size() - m_read_offset;
    }

    size_t read_buffer_t::size() const
    {
        return m_buffer.size();
    }

    uint16_t read_buffer_t::read_uint16()
    {
        return read<uint16_t>();
    }

    uint32_t read_buffer_t::read_uint32()
    {
        return read<uint32_t>();
    }

    uint64_t read_buffer_t::read_uint64()
    {
        return read<uint64_t>();
    }

    int16_t read_buffer_t::read_int16()
    {
        return read<int16_t>();
    }

    int32_t read_buffer_t::read_int32()
    {
        return read<int32_t>();
    }

    int64_t read_buffer_t::read_int64()
    {
        return read<int64_t>();
    }

    buffer_span_t read_buffer_t::read_buffer(size_t length)
    {
        if (m_read_offset >= m_buffer.size())
        {
            return buffer_span_t();
        }

        const size_t remaining = m_buffer.size() - m_read_offset;
        const size_t read_length = (sizeof(discnet::byte_t) * length);
        if (remaining > read_length)
        {
            buffer_span_t result{&m_buffer[m_read_offset], read_length};
            m_read_offset += read_length;
            return result;
        }

        return buffer_span_t();
    }

    void read_buffer_t::reset_read()
    {
        m_read_offset = 0;
    }

    template <typename type_t>
    type_t read_buffer_t::read()
    {
        type_t result = 0;
        if (m_read_offset >= m_buffer.size())
        {
            return 0;
        }

        const size_t remaining = m_buffer.size() - m_read_offset;
        if (remaining >= sizeof(type_t))
        {
            result = (type_t&)m_buffer[m_read_offset];
            m_read_offset += sizeof(type_t);
        }

        return result;
    }
} // !namespace discnet::network