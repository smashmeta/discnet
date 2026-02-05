/*
 *
 */

#pragma once

#include <format>
#include <discnet/typedefs.hpp>


namespace discnet::sim::logic
{
    using switch_identifier = uint16_t;
    static uint16_t s_ip_segement = 1;

    class network_switch
    {
    public:
        network_switch(switch_identifier id)
            : m_id(id), m_subnet_ip(std::format("192.200.{}", s_ip_segement++)), m_next_valid_ip(1)
        {
            // nothing for now
        }

        std::string get_subnet() const
        {
            return m_subnet_ip;
        }

        discnet::address_t create_ip() 
        {
            return boost::asio::ip::make_address_v4(std::format("{}.{}", m_subnet_ip, m_next_valid_ip++));
        }
    private:
        switch_identifier m_id;
        std::string m_subnet_ip;
        uint16_t m_next_valid_ip;
    };

    using shared_network_switch = std::shared_ptr<network_switch>;
} // !namespace discnet::sim::logic