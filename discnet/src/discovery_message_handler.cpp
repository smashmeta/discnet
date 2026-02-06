/*
 *
 */

#include <discnet/discovery_message_handler.hpp>


namespace discnet
{
    discovery_message_handler::discovery_message_handler(discnet::network::shared_network_handler network_handler, shared_route_manager route_manager)
        : m_route_manager(route_manager)
    {
        network_handler->e_discovery_message_received.connect(
            std::bind(&discovery_message_handler::handle_discovery_message, this, std::placeholders::_1, std::placeholders::_2));
    }

    void discovery_message_handler::handle_discovery_message(const network::messages::discovery_message_t& message, const network::network_info_t& network_info)
    {
        m_route_manager->process_discovery_message(message, network_info);
    }
} // !namespace discnet