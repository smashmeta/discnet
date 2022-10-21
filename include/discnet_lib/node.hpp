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

    using jumps_t = std::vector<uint16_t>;

    struct node_t
    {
        node_identifier_t m_identifier = init_required;
        jumps_t m_jumps = {};
    };

    using nodes_vector_t = std::vector<node_t>;
}