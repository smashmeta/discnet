/*
 *
 */

#pragma once

#include <expected>
#include <discnet/typedefs.hpp>

namespace discnet::app
{
     struct configuration_t 
     {
        uint16_t m_node_id;
        address_t m_multicast_address;
        port_type_t m_multicast_port;
     };

     bool initialize_console_logger();
     
     std::expected<configuration_t, std::string> get_configuration(int arguments_count, const char** arguments_vector);
} // !namespace discnet::app