/*
 *
 */

#pragma once

#include <memory>
#include <discnet/typedefs.hpp>
#include <discnet/application/configuration.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/network/network_handler.hpp>
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

    using instance_identifier = uint16_t;

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
        ~discnet_node();

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

    using switch_identifier = uint16_t;

    static uint16_t s_ip_segement = 1;
    class network_switch
    {
        
    public:
        network_switch(switch_identifier id)
            : m_id(id), m_subnet_ip(std::format("192.200.{}", s_ip_segement++)), m_next_valid_ip(1)
        {
            // nothing for now
        }

        std::string get_subnet() const
        {
            return m_subnet_ip;
        }

        discnet::address_t create_ip() 
        {
            return boost::asio::ip::make_address_v4(std::format("{}.{}", m_subnet_ip, m_next_valid_ip++));
        }
    private:
        switch_identifier m_id;
        std::string m_subnet_ip;
        uint16_t m_next_valid_ip;
    };

    using shared_network_switch = std::shared_ptr<network_switch>;

    class network_traffic_manager
    {
    public:
        void data_sent(const uint16_t node_id, const network::udp_info_t& info, const discnet::network::buffer_t& buffer);
        void data_sent(const uint16_t node_id, const network::udp_info_t& info, const discnet::network::buffer_t& buffer, const discnet::address_t& recipient);

        void register_client(const uint16_t node_id, network::shared_udp_client client)
        {
            auto itr_client = m_clients.find(node_id);
            if (itr_client != m_clients.end())
            {
                itr_client->second.push_back(client);
            }
            else
            {
                m_clients.insert({node_id, {client}});
            }
        }

        void set_log_handle(QTextEdit* log_handle)
        {
            m_network_logger = spdlog::qt_logger_mt("sim_network_log", log_handle);
        }

        bool add_switch(const network_switch& val)
        {
            auto existing = m_switches.find(val.get_subnet());
            if (existing == m_switches.end())
            {
                m_switches.insert({val.get_subnet(), val});
            }

            return false;
        }
    private:
        std::shared_ptr<spdlog::logger> m_network_logger;
        std::map<uint16_t, std::vector<network::shared_udp_client>> m_clients;
        std::map<std::string, network_switch> m_switches;
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

        shared_discnet_node find_node(const uint16_t node_id)
        {
            auto itr_node = m_nodes.find(node_id);
            if (itr_node != m_nodes.end())
            {
                return itr_node->second;
            }

            return {};
        }

        instance_identifier add_instance(const discnet::application::configuration_t& configuration, QTextEdit* log_handle)
        {
            auto node = std::make_shared<discnet::discnet_node>(configuration, m_network_traffic_manager, log_handle);
            node->initialize();
            
            std::lock_guard<std::mutex> lock {m_mutex};
            static uint32_t sequence_number = 1;
            instance_identifier node_id = configuration.m_node_id;
            m_nodes.insert(std::make_pair(node_id, node));
            return node_id;
        }

        bool add_switch(switch_identifier id)
        {
            return m_network_traffic_manager->add_switch(network_switch(id));
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
