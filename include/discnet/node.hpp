/*
 *
 */

#pragma once

#include <discnet/typedefs.hpp>

namespace discnet
{
    struct node_identifier_t
    {
        bool operator==(const node_identifier_t&) const = default;
        uint16_t m_id = 0;
        address_t m_address;
    };
} // !namespace discnet