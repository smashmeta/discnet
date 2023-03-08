/*
 *
 */

#pragma once

#include <expected>
#include <discnet/typedefs.hpp>
#include <discnet/discnet.hpp>

namespace discnet::application
{
     struct configuration_t 
     {
        uint16_t m_node_id;
        address_t m_multicast_address;
        port_type_t m_multicast_port;
     };

     DISCNET_EXPORT bool initialize_console_logger();
     
     DISCNET_EXPORT std::expected<configuration_t, std::string> get_configuration(int arguments_count, const char** arguments_vector);
} // !namespace discnet::application