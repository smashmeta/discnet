/*
 *
 */

#pragma once

#include <expected>
#include <discnet/typedefs.hpp>
#include <discnet/discnet.hpp>
#include <spdlog/spdlog.h>

namespace discnet::application
{
     struct configuration_t 
     {
        uint16_t m_node_id;
        address_t m_multicast_address;
        port_type_t m_multicast_port;
        std::string m_log_instance_id;
     };
     
     using expected_configuration_t = std::expected<configuration_t, std::string>;

     DISCNET_EXPORT expected_configuration_t get_configuration(int arguments_count, const char** arguments_vector);
} // !namespace discnet::application