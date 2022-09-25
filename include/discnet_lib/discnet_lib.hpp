/*
 *
 */

#pragma once

#include <string>
#include <span>
#include <cstddef>
#include <chrono>
#include <map>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/signals2.hpp>
#include <memory>

#ifdef DISCNET_DLL
#  define DISCNET_EXPORT __declspec(dllexport)
#else
#  define DISCNET_EXPORT __declspec(dllimport)
#endif

namespace discnet
{
    typedef boost::asio::ip::address_v4 address_v4_t;
    typedef std::chrono::system_clock::time_point time_point_t;
    typedef boost::uuids::uuid uuid_t;

    typedef std::pair<address_v4_t, address_v4_t> address_mask_v4_t; 

    /*
        guid: {154EA313-6D41-415A-B007-BBB7AD740F1F}
        mac_address: 3C:A9:F4:3C:1F:00
        index: 4
        name: Ethernet
        description: Intel(R) Centrino(R) Ultimate-N 6300 AGN
        enabled: true
        address_list: { [192.169.10.10, 255.255.255.0], [192.200.10.10, 255.255.255.0] }
        gateway: 192.200.10.1
     */
    struct adapter_t
    {   
        uuid_t m_guid;
        std::string m_mac_address;
        uint8_t m_index;
        std::string m_name;
        std::string m_description;
        bool m_enabled;
        std::list<address_mask_v4_t> m_address_list;
        address_v4_t m_gateway;
    };

    class adapter_fetcher
    {
    public:
        virtual ~adapter_fetcher() {}
        virtual std::vector<adapter_t> get_adapters() = 0; 
    };

    // struct windows_adapter_fetcher : public adapter_fetcher
    // {
    //     windows_adapter_fetcher(discnet::shared_wbem_consumer consumer);
    //     std::vector<adapter_t> get_adapters() override;
    // }

    class adapter_manager
    {
    public:
        boost::signals2::signal<void(const adapter_t&)> e_new;
        boost::signals2::signal<void(const adapter_t& prev, const adapter_t& curr)> e_changed;
        boost::signals2::signal<void(const adapter_t&)> e_removed;

        adapter_manager(std::unique_ptr<adapter_fetcher> fetcher)
            : m_fetcher{std::move(fetcher)} 
        {
            // nothing for now
        }

        bool is_equal(const adapter_t& lhs, const adapter_t& rhs)
        {
            return  lhs.m_guid == rhs.m_guid &&
                    lhs.m_mac_address == rhs.m_mac_address &&
                    lhs.m_name == rhs.m_name && 
                    lhs.m_index == rhs.m_index &&
                    lhs.m_description == rhs.m_description &&
                    lhs.m_enabled == rhs.m_enabled &&
                    lhs.m_address_list == rhs.m_address_list;
        }

        void update()
        {
            auto current_adapters = m_fetcher->get_adapters();
            for (const adapter_t& adapter : current_adapters)
            {
                auto existing_id = std::find_if(m_adapters.begin(), m_adapters.end(), 
                    [&](const std::pair<uuid_t, adapter_t>& pair){ return pair.first == adapter.m_guid; });
                
                // found existing entry for adapater
                if (existing_id != m_adapters.end())
                {
                    // check for changes to adapter
                    if (!is_equal(adapter, existing_id->second))
                    {
                        e_changed(existing_id->second, adapter);
                    }
                }
                else
                {
                    // new adapter detected
                    m_adapters.insert({adapter.m_guid, adapter});
                    e_new(adapter);
                }
            }

            typedef std::map<uuid_t, adapter_t> adapters_map_t;
            for (adapters_map_t::const_iterator adapter_itr = m_adapters.cbegin(); adapter_itr != m_adapters.cend();)
            {
                auto existing_id = std::find_if(current_adapters.begin(), current_adapters.end(), 
                    [&](const adapter_t& val){ return val.m_guid == adapter_itr->first; });

                if (existing_id == current_adapters.end())
                {
                    // removed adapter detected
                    e_removed(adapter_itr->second);
                    adapter_itr = m_adapters.erase(adapter_itr);
                }
                else
                {
                    adapter_itr = std::next(adapter_itr);
                }
            }
        }
    protected:
        std::unique_ptr<adapter_fetcher> m_fetcher;
        std::map<uuid_t, adapter_t> m_adapters;
    };

    struct node_identifier
    {
        uint16_t m_id;
        address_v4_t m_address;
    };

    struct route_identifier
    {
        node_identifier m_node;
        address_v4_t m_adapter;
        address_v4_t m_reporter;
    };

    /* 
        { 
            route_id: [ node: { 1010, 192.169.10.10 }, adapter: 192.169.10.1, reporter: 192.169.10.10 ] 
            last_tdp:  2022-12-09 14:23:11
            last_data: 1970-01-01 00:00:00 <no data received>
            persistent: false
            silent: false
            metric: 256
        }
    */
    struct route_t
    {
        route_identifier m_identifier;
        time_point_t m_last_tdp;
        time_point_t m_last_data_message;
        bool m_persistent;
        bool m_silent;
        uint8_t m_metric;
    };

    struct route_manager
    {
        void update(time_point_t current_time)
        {
            for (const auto& route : m_routes)
            {
                
            }
        }

        std::map<route_identifier, route_t> m_routes;
        std::vector<route_identifier> m_active_routes;
    };

    DISCNET_EXPORT bool is_route_online(const route_t& route);
    DISCNET_EXPORT bool is_direct_node(const route_identifier& route);
    DISCNET_EXPORT bool contains(const std::span<route_identifier>& routes, const route_identifier& route);

    DISCNET_EXPORT std::string bytes_to_hex_string(const std::span<std::byte>& buffer);
} // !namesapce discnet