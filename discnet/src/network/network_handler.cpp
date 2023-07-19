/*
 *
 */

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <whatlog/logger.hpp>
#include <discnet/network/network_handler.hpp>

namespace discnet::network 
{
    network_handler::network_handler(shared_adapter_manager adapter_manager, const discnet::application::configuration_t& configuration, discnet::shared_io_context io_context)
        : m_adapter_manager(adapter_manager), m_configuration(configuration), m_io_context(io_context)
    {
        m_adapter_manager->e_new.connect(std::bind(&network_handler::adapter_added, this, std::placeholders::_1));
        m_adapter_manager->e_changed.connect(std::bind(&network_handler::adapter_changed, this, std::placeholders::_1, std::placeholders::_2));
        m_adapter_manager->e_removed.connect(std::bind(&network_handler::adapter_removed, this, std::placeholders::_1));
    }

    void network_handler::transmit_multicast(const discnet::adapter_t& adapter, const discnet::network::messages::message_list_t& messages)
    {
        whatlog::logger log("multicast_handler::transmit_multicast");
        
        auto itr_client = m_clients.find(adapter.m_guid);
        if (itr_client == m_clients.end())
        {
            log.error("failed to find client for adapter {} - message(s) dropped.", adapter.m_name);
            return;
        }
        
        discnet::network::buffer_t buffer(4096);
        auto success = discnet::network::messages::packet_codec_t::encode(buffer, messages);
        if (!success)
        {
            log.error("failed to encode messages to a valid packet.");
        }

        auto& [uuid, udp_client] = *itr_client;
        udp_client.m_multicast->write(buffer);
    }

    void network_handler::transmit_unicast(const discnet::adapter_t& adapter, const::discnet::address_t& recipient, discnet::network::messages::message_list_t& messages)
    {
        whatlog::logger log("network_handler::transmit_unicast");

        auto itr_client = m_clients.find(adapter.m_guid);
        if (itr_client == m_clients.end())
        {
            log.error("failed to find client for adapter {} - message(s) dropped.", adapter.m_name);
            return;
        }

        discnet::network::buffer_t buffer(4096);
        auto success = discnet::network::messages::packet_codec_t::encode(buffer, messages);
        if (!success)
        {
            log.error("failed to encode messages to a valid packet.");
        }

        auto& [uuid, udp_client] = *itr_client;
        udp_client.m_unicast->write(recipient, buffer);
    }

    void network_handler::update()
    {
        whatlog::logger log("network_handler::update");

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
                        log.info("now listening for messages on adapter {}.", client.m_multicast->info().m_adapter_address.to_string());
                        m_adapter_manager->update_multicast_present(client.m_adapter_identifier, true);
                        m_clients.insert(std::pair{client.m_adapter_identifier, client});
                    }
                    else
                    {
                        log.error("failed to create client. message: {}.", result.error());
                    }
                }
            }
        }

        for (auto& client : m_clients | std::views::values)
        {
            network_info_t network_info;
            network_info.m_adapter = client.m_multicast->info().m_adapter_address;
            network_info.m_receiver = client.m_multicast->info().m_multicast_address;
            network_info.m_reception_time = discnet::time_point_t::clock::now();

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
        whatlog::logger log("network_handler::remove_client");

        auto itr_client = m_clients.find(adapter.m_guid);
        if (itr_client != m_clients.end())
        {
            auto& [uuid, client] = *itr_client;
            std::string adapter_guid_str = boost::lexical_cast<std::string>(uuid);
            log.info("removing client from adapter (name: {}, guid: {}).", adapter.m_name, adapter_guid_str);
            client.m_multicast->close();
            client.m_unicast->close();
            m_clients.erase(itr_client);
        }
    }

    network_handler::network_client_result_t network_handler::process_adapter(network_client_t client)
    {
        whatlog::rename_thread(GetCurrentThread(), "process_adapter");
        whatlog::logger log("network_handler::process_adapter");

        bool unicast_enabled = client.m_unicast->open();
        bool multicast_enabled = client.m_multicast->open();
        size_t index = 1;
        discnet::time_point_t start_time = discnet::time_point_t::clock::now();
        discnet::time_point_t current_time = start_time;
        discnet::time_point_t timeout = start_time + std::chrono::seconds(15); 
        while ((!unicast_enabled || !multicast_enabled) && current_time < timeout)
        {
            // log.info("retry connect #{}.", index);
            // give the OS some time to initialize the adapter before we start listening
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!multicast_enabled)
            {
                multicast_enabled = client.m_multicast->open();
            }
            if (!unicast_enabled)
            {
                unicast_enabled = client.m_unicast->open();
            }
            
            current_time = discnet::time_point_t::clock::now();
            ++index;
        }

        if (!multicast_enabled)
        {
            return std::unexpected("failed to start multicast client. dropping client for adapter");
        }

        if (!unicast_enabled)
        {
            return std::unexpected("failed to start unicast client. dropping client for adapter");            
        }

        return client;
    }

    void network_handler::add_client(const discnet::adapter_t& adapter)
    {
        whatlog::logger log("network_handler::add_client");

        if (!adapter.m_enabled)
        {
            log.info("skipping adapter {} because it is disabled.", adapter.m_name);
            return;
        }

        if (adapter.m_address_list.empty())
        {
            log.info("skipping adapter {} because it is missing ipv4 address.", adapter.m_name);
            return;
        }

        discnet::network::multicast_info_t multicast_info;
        multicast_info.m_adapter_address = adapter.m_address_list.front().first;
        multicast_info.m_multicast_address = m_configuration.m_multicast_address;
        multicast_info.m_multicast_port = m_configuration.m_multicast_port;

        discnet::network::unicast_info_t unicast_info;
        unicast_info.m_address = adapter.m_address_list.front().first;
        unicast_info.m_port = m_configuration.m_multicast_port + 1;

        network_client_t client;
        client.m_data_handler = std::make_shared<discnet::network::data_handler>(4095);
        client.m_multicast = discnet::network::multicast_client::create(m_io_context, multicast_info, client.m_data_handler);
        client.m_unicast = discnet::network::unicast_client::create(m_io_context, unicast_info, client.m_data_handler);
        client.m_adapter_identifier = adapter.m_guid;

        {
            std::lock_guard<std::mutex> guard {m_adapter_init_list_mutex};
            m_adapter_init_list.push_back(std::async(std::launch::async, &network_handler::process_adapter, this, client));
        }
    }

    void network_handler::adapter_added(const adapter_t& adapter)
    {
        whatlog::logger log("network_handler::adapter_added");
        
        std::string adapter_guid_str = boost::lexical_cast<std::string>(adapter.m_guid);
        log.info("new adapter detected. Name: {}, guid: {}, mac: {}.", adapter.m_name, adapter_guid_str, adapter.m_mac_address);
        add_client(adapter);
    }

    void network_handler::adapter_changed(const adapter_t& previous_adapter, const adapter_t& current_adapter)
    {
        whatlog::logger log("network_handler::adapter_changed");
        
        bool client_exists = m_clients.find(current_adapter.m_guid) != m_clients.end();
        if (client_exists)
        {
            bool ip_address_changed = previous_adapter.m_address_list != current_adapter.m_address_list;
            if (!current_adapter.m_multicast_enabled)
            {
                log.info("adapter {} multicast disabled. removing client.", current_adapter.m_name);
                remove_client(previous_adapter);
            }
            else if (ip_address_changed)
            {
                log.info("adapter {} ip-address chaned. re-creating client.", current_adapter.m_name);
                remove_client(previous_adapter);
                add_client(current_adapter);
            }
        }
        else 
        {
            if (current_adapter.m_multicast_enabled)
            {
                log.info("unknown adapter {} appeared. adding client.", current_adapter.m_name);
                add_client(current_adapter);
            }
        }
    }

    void network_handler::adapter_removed(const adapter_t& adapter)
    {
        whatlog::logger log("network_handler::adapter_removed");
        remove_client(adapter);
    }
} // ! namespace discnet::network