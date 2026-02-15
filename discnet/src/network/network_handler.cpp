/*
 *
 */

#include "discnet/adapter.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional>
#include <spdlog/spdlog.h>
#include <discnet/network/network_handler.hpp>

namespace discnet::network 
{
    namespace 
    {
        bool equals(const network_client_t& lhs, const adapter_guid_t& adapter_id)
        {
            return lhs.m_adapter_identifier == adapter_id;
        }
    } // !anonymous namespace

    client_creator::client_creator(const discnet::application::configuration_t& configuration, discnet::shared_io_context io_context)
        : m_configuration(configuration), m_logger(spdlog::get(configuration.m_log_instance_id)), m_io_context(io_context)
    {
        // nothing for now
    }

    shared_udp_client client_creator::create(const udp_info_t& info, const data_received_func& callback_func)
    {
        return discnet::network::udp_client::create(m_configuration, m_io_context, info, callback_func);
    }

    network_handler::network_handler(const discnet::application::configuration_t& configuration, shared_adapter_manager adapter_manager, shared_client_creator client_creator)
        : m_configuration(configuration), m_logger(spdlog::get(configuration.m_log_instance_id)), m_adapter_manager(adapter_manager), m_client_creator(client_creator)
    {
        m_adapter_manager->e_new.connect(std::bind(&network_handler::adapter_added, this, std::placeholders::_1));
        m_adapter_manager->e_changed.connect(std::bind(&network_handler::adapter_changed, this, std::placeholders::_1, std::placeholders::_2));
        m_adapter_manager->e_removed.connect(std::bind(&network_handler::adapter_removed, this, std::placeholders::_1));
    }

    network_handler::~network_handler()
    {
        m_adapter_manager->e_new.disconnect_all_slots();
        m_adapter_manager->e_changed.disconnect_all_slots();
        m_adapter_manager->e_removed.disconnect_all_slots();
    }

    network_handler::network_clients_t network_handler::clients() const
    {
        return m_clients;
    }

    void network_handler::transmit_multicast(const discnet::adapter_t& adapter, const discnet::network::messages::message_list_t& messages)
    {
        auto adapter_check_func = std::bind(&equals,std::placeholders::_1, adapter.m_mac_address);
        auto itr_client = std::find_if(m_clients.begin(), m_clients.end(), adapter_check_func);
        if (itr_client == m_clients.end())
        {
            m_logger->error("multicast - failed to find client for adapter {} - message(s) dropped.", adapter.m_name);
            return;
        }

        auto& client = itr_client->m_client;
        if (client && client->info().m_multicast != discnet::address_t::any())
        {
            discnet::network::buffer_t buffer(4096);
            auto success = discnet::network::messages::packet_codec_t::encode(buffer, messages);
            if (!success)
            {
                m_logger->error("failed to encode messages to a valid packet.");
            }

            client->write(buffer);
        }
    }

    void network_handler::transmit_unicast(const discnet::adapter_t& adapter, const::discnet::address_t& recipient, discnet::network::messages::message_list_t& messages)
    {
        auto adapter_check_func = std::bind(&equals,std::placeholders::_1, adapter.m_mac_address);
        auto itr_client = std::find_if(m_clients.begin(), m_clients.end(), adapter_check_func);
        if (itr_client == m_clients.end())
        {
            m_logger->error("unicast - failed to find client for adapter {} - message(s) dropped.", adapter.m_name);
            return;
        }

        auto& client = itr_client->m_client;
        if (client)
        {
            discnet::network::buffer_t buffer(4096);
            auto success = discnet::network::messages::packet_codec_t::encode(buffer, messages);
            if (!success)
            {
                m_logger->error("failed to encode messages to a valid packet.");
            }

            client->write(buffer, recipient);
        }
    }

    void network_handler::update(const discnet::time_point_t& current)
    {
        if (m_adapter_init_list.size() > 0)
        {   // see if any new clients have been established
            std::lock_guard<std::mutex> guard {m_adapter_init_list_mutex};
            for (auto& adapter_connect : m_adapter_init_list)
            {
                if (adapter_connect.valid() && adapter_connect.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    auto result = adapter_connect.get();
                    if (result)
                    {
                        network_client_t client = result.value();
                        m_logger->info("now listening for messages on adapter {}.", client.m_client->info().m_adapter.to_string());
                        m_clients.push_back(client);
                    }
                    else
                    {
                        m_logger->error("failed to create client. message: {}.", result.error());
                    }
                }
            }
        }

        for (auto& client : m_clients)
        {
            network_info_t network_info;
            network_info.m_adapter = client.m_client->info().m_adapter;
            network_info.m_reception_time = current;

            auto streams = client.m_data_handler->process();
            for (const data_stream_packets_t& stream : streams)
            {
                for (const packet_t& packet : stream.m_packets)
                {
                    network_info.m_sender = stream.m_identifier.m_sender_ip;
                    network_info.m_receiver = stream.m_identifier.m_recipient_ip;

                    for (const message_variant_t& message : packet.m_messages)
                    {
                        if (std::holds_alternative<discovery_message_t>(message))
                        {
                            auto discovery_message = std::get<discovery_message_t>(message);
                            e_discovery_message_received(discovery_message, network_info);
                        }
                        else if (std::holds_alternative<data_message_t>(message))
                        {
                            auto data_message = std::get<data_message_t>(message);
                            e_data_message_received(data_message, network_info);
                        }
                    }
                }
            }
        }
    }

    void network_handler::remove_client(const discnet::adapter_t& adapter)
    {
        auto adapter_check_func = std::bind(&equals,std::placeholders::_1, adapter.m_mac_address);
        auto itr_client = std::find_if(m_clients.begin(), m_clients.end(), adapter_check_func);
        if (itr_client != m_clients.end())
        {
            auto& client = *itr_client;
            m_logger->info("removing client from adapter (name: {}, guid: {}).", adapter.m_name, adapter.m_mac_address);
            client.m_client->close();
            m_clients.erase(itr_client);
        }
    }

    network_handler::network_client_result_t network_handler::process_adapter(network_client_t client)
    {
        bool udp_enabled = client.m_client->open();
        size_t retry_count = 0;
        discnet::time_point_t start_time = discnet::time_point_t::clock::now();
        discnet::time_point_t current_time = start_time;
        discnet::time_point_t timeout = start_time + std::chrono::seconds(15); 
        while (!udp_enabled && current_time < timeout)
        {
            m_logger->info("retry connect #{}.", ++retry_count);
            // give the OS some time to initialize the adapter before we start listening
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!udp_enabled)
            {
                udp_enabled = client.m_client->open();
            }
            
            current_time = discnet::time_point_t::clock::now();
        }

        if (!udp_enabled)
        {
            client.m_client.reset();
            return std::unexpected("failed to start udp client. dropping client for adapter");            
        }

        return client;
    }

    void network_handler::add_client(const discnet::adapter_t& adapter)
    {
        if (!adapter.m_enabled)
        {
            m_logger->info("skipping adapter {} because it is disabled.", adapter.m_name);
            return;
        }

        if (adapter.m_address_list.empty())
        {
            m_logger->info("skipping adapter {} because it is missing ipv4 address.", adapter.m_name);
            return;
        }

        if (adapter.m_loopback)
        {
            m_logger->info("skipping adapter {} because it is a loopback adapter.", adapter.m_name);
            return;
        }

        discnet::network::udp_info_t udp_info;
        udp_info.m_adapter = adapter.m_address_list.front().first;
        udp_info.m_multicast = m_configuration.m_multicast_address;
        udp_info.m_port = m_configuration.m_multicast_port;

        network_client_t client;
        client.m_data_handler = std::make_shared<discnet::network::data_handler>(4095);
        auto data_received_callback_func = std::bind(&discnet::network::data_handler::handle_receive, client.m_data_handler, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        client.m_client = m_client_creator->create(udp_info, data_received_callback_func);
        client.m_adapter_identifier = adapter.m_mac_address;

        {
            std::lock_guard<std::mutex> guard {m_adapter_init_list_mutex};
            m_adapter_init_list.push_back(std::async(std::launch::async, &network_handler::process_adapter, this, client));
        }
    }

    void network_handler::adapter_added(const adapter_t& adapter)
    {
        std::string adapter_guid_str = boost::lexical_cast<std::string>(adapter.m_guid);
        m_logger->info("new adapter detected. Name: {}, guid: {}, mac: {}.", adapter.m_name, adapter_guid_str, adapter.m_mac_address);
        add_client(adapter);
    }

    void network_handler::adapter_changed(const adapter_t& previous_adapter, const adapter_t& current_adapter)
    {
        auto adapter_check_func = std::bind(&equals,std::placeholders::_1, current_adapter.m_mac_address);
        auto itr_client = std::find_if(m_clients.begin(), m_clients.end(), adapter_check_func);
        if (itr_client != m_clients.end())
        {
            bool ip_address_changed = previous_adapter.m_address_list != current_adapter.m_address_list;
            if (ip_address_changed)
            {
                m_logger->info("adapter {} ip-address chaned. re-creating client.", current_adapter.m_name);
                remove_client(previous_adapter);
                add_client(current_adapter);
            }
        }
    }

    void network_handler::adapter_removed(const adapter_t& adapter)
    {
        remove_client(adapter);
    }
} // ! namespace discnet::network