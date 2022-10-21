/*
 *
 */

#pragma once

#include <discnet_lib/typedefs.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace discnet
{
    typedef uuid_t adapter_identifier_t;

    /*
        guid: {154EA313-6D41-415A-B007-BBB7AD740F1F}
        mac_address: 3C:A9:F4:3C:1F:00
        index: 4
        name: Ethernet
        description: Intel(R) Centrino(R) Ultimate-N 6300 AGN
        enabled: true
        address_list: { [192.169.10.10, 255.255.255.0], [192.200.10.10, 255.255.255.0] }
        gateway: 192.200.10.1
     */
    struct adapter_t
    {   
        adapter_t() = default;
        adapter_t(const adapter_t& val) = default;
        
        adapter_identifier_t m_guid = adapter_identifier_t();
        std::string m_mac_address = "";
        uint8_t m_index = 0;
        std::string m_name = "";
        std::string m_description = "";
        bool m_enabled = false;
        std::list<address_mask_v4_t> m_address_list;
        address_v4_t m_gateway = address_v4_t::any();
    };
}