/*
 *
 */

#pragma once

#include <memory>
#include <discnet/typedefs.hpp>
#include <discnet/application/configuration.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/network/multicast_client.hpp>
#include <discnet/network/unicast_client.hpp>
#include <QTextEdit>
#include <spdlog/sinks/qt_sinks.h>

namespace discnet
{
    class adapter_manager;
    using shared_adapter_manager = std::shared_ptr<adapter_manager>;

    class route_manager;
    using shared_route_manager = std::shared_ptr<route_manager>;

namespace network
{
    class network_handler;
    using shared_network_handler = std::shared_ptr<network_handler>;
} // !namespace network

    struct asio_context_t;
    using shared_asio_context_t = std::shared_ptr<asio_context_t>;

    class discovery_message_handler;
    using shared_discovery_message_handler = std::shared_ptr<discovery_message_handler>;

    class transmission_handler;
    using shared_transmission_handler = std::shared_ptr<transmission_handler>;

    using instance_identifier = std::string;

    class simulator_adapter_fetcher : public adapter_fetcher
    {
    public:
        simulator_adapter_fetcher() {}
        void add_adapter(const adapter_t& adapter) 
        { 
            std::lock_guard<std::mutex> lock{m_mutex};
            m_adapters.push_back(adapter); 
        }
        std::vector<adapter_t> get_adapters() 
        { 
            std::vector<adapter_t> copy;
            {
                std::lock_guard<std::mutex> lock{m_mutex};
                copy = m_adapters;
            }
            return copy; 
        }
    private:
        std::mutex m_mutex;
        std::vector<adapter_t> m_adapters;
    };

    using shared_simulator_adapter_fetcher = std::shared_ptr<simulator_adapter_fetcher>;

    class network_traffic_manager;
    using shared_network_traffic_manager = std::shared_ptr<network_traffic_manager>;

    class discnet_node
    {
    public:
        discnet_node(const application::configuration_t& configuration, const shared_network_traffic_manager& ntm, QTextEdit* text_edit);

        void add_adapter(const adapter_t& adapter);
        
        bool initialize();
        void update(time_point_t current_time);
    private:
        application::configuration_t m_configuration;
        shared_asio_context_t m_asio_context;
        shared_simulator_adapter_fetcher m_adapter_fetcher;
        discnet::shared_adapter_manager m_adapter_manager;
        discnet::shared_route_manager m_route_manager;
        discnet::network::shared_network_handler m_network_handler;
        shared_discovery_message_handler m_discovery_message_handler;
        shared_transmission_handler m_transmission_handler;
        discnet::application::shared_loggers m_loggers;
        shared_network_traffic_manager m_network_traffic_manager;
    };

    using shared_discnet_node = std::shared_ptr<discnet_node>;

    class network_traffic_manager
    {
    public:
        void data_sent([[maybe_unused]] const uint16_t node_id, [[maybe_unused]] const network::multicast_info_t& info, [[maybe_unused]] const discnet::network::buffer_t& buffer)
        {
            if (m_network_logger)
            {
                std::string log_entry = std::format("node: {}, adapter: {}, multicast: [addr: {}, port: {}] - {}",
                    node_id, info.m_adapter_address.to_string(), info.m_multicast_address.to_string(), info.m_multicast_port, discnet::bytes_to_hex_string(buffer.data()));
                m_network_logger->info(log_entry);
            }
        }

        void set_log_handle(QTextEdit* log_handle)
        {
            m_network_logger = spdlog::qt_logger_mt("sim_network_log", log_handle);
        }
    private:
        std::shared_ptr<spdlog::logger> m_network_logger;
    };

    using shared_network_traffic_manager = std::shared_ptr<network_traffic_manager>;

    class simulator
    {
    public:
        simulator()
            : m_network_traffic_manager(std::make_shared<network_traffic_manager>())
        {
            // nothing for now
        }

        instance_identifier add_instance(const discnet::application::configuration_t& configuration, QTextEdit* log_handle)
        {
            auto node = std::make_shared<discnet::discnet_node>(configuration, m_network_traffic_manager, log_handle);
            node->initialize();
            
            static int ip_addr = 1;
            discnet::adapter_t tmp;
            tmp.m_guid = "154EA313-6D41-415A-B007-BBB7AD740F1F";
            tmp.m_mac_address = "3C:A9:F4:3C:1F:00";
            tmp.m_index = 0;
            tmp.m_name = "dummy_adapter";
            tmp.m_description = "[description]";
            tmp.m_loopback = false;
            tmp.m_enabled = true;
            tmp.m_address_list = { discnet::address_mask_t{boost::asio::ip::make_address_v4(std::format("192.200.1.{}", ip_addr++)), boost::asio::ip::make_address_v4("255.255.255.0")} };
            tmp.m_gateway = discnet::address_t::any();
            tmp.m_mtu = 1024;
            node->add_adapter(tmp);

            std::lock_guard<std::mutex> lock {m_mutex};
            static uint32_t sequence_number = 1;
            instance_identifier node_id = std::format("node_{}", configuration.m_node_id);
            m_nodes.insert(std::make_pair(node_id, node));
            return node_id;
        }
        
        bool remove_instance(const instance_identifier& id)
        {
            std::lock_guard<std::mutex> lock {m_mutex};
            auto instance_itr = m_nodes.find(id);
            if (instance_itr != m_nodes.end())
            {
                m_nodes.erase(instance_itr);
                return true;
            }

            return false;
        }

        void set_traffic_manager_log_handle(QTextEdit* log_handle)
        {
            m_network_traffic_manager->set_log_handle(log_handle);
        }

        void update(discnet::time_point_t current_time)
        {
            std::lock_guard<std::mutex> lock {m_mutex};
            for (auto& [id, node] : m_nodes)
            {
                node->update(current_time);
            }
        }
    private:
        std::mutex m_mutex;
        std::map<instance_identifier, discnet::shared_discnet_node> m_nodes;
        shared_network_traffic_manager m_network_traffic_manager;
        
    };
} // !namespace discnet
