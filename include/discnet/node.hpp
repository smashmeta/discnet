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
        uint16_t m_id = init_required;
        address_t m_address = init_required;
    };
} // !namespace discnet