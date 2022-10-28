/*
 *
 */

#include <discnet_lib/network/buffer.hpp>


namespace discnet::network
{
    namespace
    {
        // nothing for now
    } // !anoynmous namespace
     
    buffer_t::buffer_t(size_t size)
        : m_write_offset(0), m_read_offset(0), m_buffer(size, 0)
    {
        // nothing for now
    }

    buffer_span_t buffer_t::data() const
    {
        return buffer_span_t(m_buffer.data(), m_write_offset);
    }

    size_t buffer_t::remaining_bytes() const
    {
        return m_buffer.size() - m_write_offset;
    }

    size_t buffer_t::appended_bytes() const
    {
        return m_write_offset;
    }

    size_t buffer_t::bytes_left_to_read() const
    {
        if (m_read_offset >= m_write_offset)
        {
            return 0;
        }

        return m_write_offset - m_read_offset;
    }
    
    const_buffer_t buffer_t::const_buffer() const
    {
        return const_buffer_t((void*)m_buffer.data(), m_write_offset);
    }

    template <typename type_t>
    type_t buffer_t::read()
    {
        type_t result = 0;
        if (m_read_offset >= m_write_offset)
        {
            return 0;
        }

        const size_t remaining = m_write_offset - m_read_offset;
        if (remaining > sizeof(type_t))
        {
            result = (type_t&)m_buffer[m_write_offset];
            m_write_offset += sizeof(type_t);
        }

        return result;
    }

    template <typename type_t>
    bool buffer_t::append(type_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_write_offset, &val, val_size);
        m_write_offset += val_size;

        return true;
    }

    buffer_span_t buffer_t::read_buffer(size_t length)
    {
        if (m_read_offset >= m_write_offset)
        {
            return buffer_span_t();
        }

        const size_t remaining = m_write_offset - m_read_offset;
        if (remaining > (sizeof(discnet::byte_t) * length))
        {
            return buffer_span_t(&m_buffer[m_write_offset], length);
        }

        return buffer_span_t();
    }

    bool buffer_t::append(const buffer_span_t& buffer)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = buffer.size() * sizeof(discnet::byte_t);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_write_offset, buffer.data(), val_size);
        m_write_offset += val_size;

        return true;
    }

    void buffer_t::reset_write()
    {
        m_write_offset = 0;
        m_read_offset = 0;
        std::memset(&m_buffer[0], discnet::byte_t(), m_buffer.size());
    }

    void buffer_t::reset_read()
    {
        m_read_offset = 0;
    }

    void buffer_t::resize(size_t new_size)
    {
        m_write_offset = 0;
        m_buffer.resize(new_size, discnet::byte_t());
    }

    template bool buffer_t::append<uint8_t>(uint8_t val);
    template bool buffer_t::append<uint16_t>(uint16_t val);
    template bool buffer_t::append<uint32_t>(uint32_t val);
    template bool buffer_t::append<uint64_t>(uint64_t val);
    template bool buffer_t::append<int8_t>(int8_t val);
    template bool buffer_t::append<int16_t>(int16_t val);
    template bool buffer_t::append<int32_t>(int32_t val);
    template bool buffer_t::append<int64_t>(int64_t val);
} // !namespace discnet::network