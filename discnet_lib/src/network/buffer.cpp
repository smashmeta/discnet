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
     
    buffer_t::buffer_t(const size_t size)
        : m_offset(0), m_buffer(size, 0)
    {
        // nothing for now
    }

    buffer_span_t buffer_t::data() const
    {
        return buffer_span_t(m_buffer.data(), m_offset);
    }

    size_t buffer_t::remaining_bytes() const
    {
        return m_buffer.size() - m_offset;
    }

    size_t buffer_t::appended_bytes() const
    {
        return m_offset;
    }
    
    const_buffer_t buffer_t::const_buffer() const
    {
        return const_buffer_t((void*)m_buffer.data(), m_offset);
    }

    bool buffer_t::append(const buffer_span_t& buffer)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = buffer.size() * sizeof(discnet::byte_t);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, buffer.data(), val_size);
        m_offset += val_size;

        return true;
    }

    bool buffer_t::append(const uint8_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, &val, val_size);
        m_offset += val_size;

        return true;
    }

    bool buffer_t::append(const int8_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, &val, val_size);
        m_offset += val_size;

        return true;
    }

    bool buffer_t::append(const uint16_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, &val, val_size);
        m_offset += val_size;

        return true;
    }

    bool buffer_t::append(const int16_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, &val, val_size);
        m_offset += val_size;

        return true;
    }

    bool buffer_t::append(const uint32_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, &val, val_size);
        m_offset += val_size;

        return true;
    }

    bool buffer_t::append(const int32_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, &val, val_size);
        m_offset += val_size;

        return true;
    }

    bool buffer_t::append(const uint64_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, &val, val_size);
        m_offset += val_size;

        return true;
    }

    bool buffer_t::append(const int64_t val)
    {
        const size_t remaining = remaining_bytes();
        const size_t val_size = sizeof(val);
        if (val_size > remaining)
        {
            return false;
        }

        std::memcpy(m_buffer.data() + m_offset, &val, val_size);
        m_offset += val_size;

        return true;
    }

    void buffer_t::reset()
    {
        m_offset = 0;
        std::memset(&m_buffer[0], discnet::byte_t(), m_buffer.size());
    }

    void buffer_t::resize(const size_t new_size)
    {
        m_offset = 0;
        m_buffer.resize(new_size, discnet::byte_t());
    }
} // !namespace discnet::network