/*
 *
 */

#pragma once

#include <discnet_lib/typedefs.hpp>

namespace discnet
{
    struct node_identifier_t
    {
        uint16_t m_id = init_required;
        address_v4_t m_address = init_required;
    };
} // !namespace discnet