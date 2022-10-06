/*
 *
 */

#pragma once

#include <discnet_lib/node.hpp>
#include <discnet_lib/discnet_lib.hpp>

namespace discnet
{
    struct discovery_message_t
    {
        typedef std::vector<node_identifier_t> nodes_vector_t;

        uint16_t m_node_id;
        nodes_vector_t m_child_nodes;
    };
} // !namespace discnet