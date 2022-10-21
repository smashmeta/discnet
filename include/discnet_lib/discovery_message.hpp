/*
 *
 */

#pragma once

#include <expected>
#include <variant>
#include <boost/endian.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <discnet_lib/discnet_lib.hpp>
#include <discnet_lib/network/buffer.hpp>
#include <discnet_lib/node.hpp>
#include <discnet_lib/typedefs.hpp>
#include <discnet_lib/network/network_info.hpp>
#include <discnet_lib/network/messages/discovery_message.hpp>

namespace discnet::network
{
    struct discovery_data_handler_t
    {
        boost::signals2::signal<void(const messages::discovery_message_t&, const network_info_t&)> e_discovery_message;
        
        bool process(std::span<discnet::byte_t> packet, const network_info_t& packet_info)
        {
            // todo: decode data packet into a discovery message.
            return false;
        }
    };
} // !namespace discnet::network