/*
 *
 */

#include <simulator/network_switch.h>


namespace discnet::sim::logic
{
    network_switch::network_switch(switch_identifier id)
        : m_id(id), m_subnet_ip(std::format("192.200.{}", s_ip_segement++)), m_next_valid_ip(2)
    {
        // nothing for now
    }

    std::string network_switch::get_subnet() const
    {
        return m_subnet_ip;
    }

    discnet::address_t network_switch::create_ip() 
    {
        return boost::asio::ip::make_address_v4(std::format("{}.{}", m_subnet_ip, m_next_valid_ip++));
    }
} // !namespace dicsnet::sim::logic