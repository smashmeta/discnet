/*
 *
 */

#include "discnet/adapter.hpp"
#include <iostream>
#include <vector>
#include <locale>
#include <codecvt>
#include <ranges>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <discnet/adapter_manager.hpp>
#include <spdlog/spdlog.h>


namespace discnet
{
    namespace 
    {
        size_t mask_to_cidr(const address_t& mask)
        {
            size_t cidr = 0;
            auto bytes = mask.to_uint();
            while (bytes)
            {
                cidr += bytes & 1;
                bytes >>= 1;
            }

            return cidr;
        }

        std::string ipv4_info(const adapter_t& lhs)
        {
            std::string result;
            for (const auto& address_mask : lhs.m_address_list)
            {
                auto& [address, mask] = address_mask;
                if (result.empty())
                {
                    result += "{";
                }
                else
                {
                    result += " ";
                }

                result += std::format("{}/{}", address.to_string(), mask_to_cidr(mask));
            }

            if (result.empty())
            {
                result += "{";
            }
                
            result += "}";

            return result;
        }

        std::vector<std::string> ipv4_diff(const adapter_t& lhs, const adapter_t& rhs)
        {
            std::vector<std::string> result;
            std::string lhs_adapter_addrs = ipv4_info(lhs);
            std::string rhs_adapters_addrs = ipv4_info(rhs);

            result.push_back(std::format("  prev: {}", lhs_adapter_addrs));
            result.push_back(std::format("  curr: {}", rhs_adapters_addrs));
            return result;
        }

        std::vector<std::string> adapter_info(const adapter_t& adapter)
        {
            std::vector<std::string> result;
            result.push_back(std::format("guid: {}", boost::lexical_cast<std::string>(adapter.m_guid)));
            result.push_back(std::format("mac: {}", adapter.m_mac_address));
            result.push_back(std::format("name: {}", adapter.m_name));
            result.push_back(std::format("index: {}", adapter.m_index));
            result.push_back(std::format("description: {}", adapter.m_description));
            result.push_back(std::format("adapter enabled: {}", adapter.m_enabled));
            result.push_back(std::format("mtu: {}", adapter.m_mtu));
            result.push_back(std::format("gateway: {}", adapter.m_gateway.to_string()));
            result.push_back(std::format("ipv4: {}", ipv4_info(adapter)));
            return result;
        }

        std::vector<std::string> adapter_diff(const adapter_t& lhs, const adapter_t& rhs)
        {
            std::vector<std::string> result;
            bool guid_changed = (lhs.m_guid != rhs.m_guid); 
            if (guid_changed) result.push_back(std::format("guid: {} => {}", boost::lexical_cast<std::string>(lhs.m_guid), boost::lexical_cast<std::string>(rhs.m_guid)));

            bool mac_address_changed = (lhs.m_mac_address != rhs.m_mac_address);
            if (mac_address_changed) result.push_back(std::format("mac: {} => {}", lhs.m_mac_address, rhs.m_mac_address));

            bool name_changed = (lhs.m_name != rhs.m_name);
            if (name_changed) result.push_back(std::format("name: {} => {}", lhs.m_name, rhs.m_name));
            
            bool index_changed = (lhs.m_index != rhs.m_index);
            if (index_changed) result.push_back(std::format("index: {} => {}", lhs.m_index, rhs.m_index));

            bool description_changed = (lhs.m_description != rhs.m_description);
            if (description_changed) result.push_back(std::format("description: {} => {}", lhs.m_description, rhs.m_description));

            bool enabled_changed = (lhs.m_enabled != rhs.m_enabled);
            if (enabled_changed) result.push_back(std::format("adapter enabled: {} => {}", lhs.m_enabled, rhs.m_enabled));

            bool mtu_changed = (lhs.m_mtu != rhs.m_mtu);
            if (mtu_changed) result.push_back(std::format("mtu: {} => {}", lhs.m_mtu, rhs.m_mtu));

            bool gateway_changed = (lhs.m_gateway != rhs.m_gateway);
            if (gateway_changed) result.push_back(std::format("gateway: {} => {}", lhs.m_gateway.to_string(), rhs.m_gateway.to_string()));

            bool address_list_changed = (lhs.m_address_list != rhs.m_address_list);
            if (address_list_changed) 
            {
                auto ip_changes = ipv4_diff(lhs, rhs);
                result.push_back(std::format("ipv4: changed (*)"));
                result.insert(result.end(), ip_changes.begin(), ip_changes.end());
            }

            return result;
        }
    } // ! anonymous namespace

    adapter_manager::adapter_manager(const discnet::application::shared_loggers& loggers, std::shared_ptr<adapter_fetcher> fetcher)
        : m_loggers(loggers), m_fetcher(fetcher) 
    {
        // nothing for now
    }

    void adapter_manager::update()
    {
        auto current_adapters = m_fetcher->get_adapters();
        for (const adapter_t& current_adapter : current_adapters)
        {
            // found existing entry for adapater
            auto existing_entry = m_adapters.find(current_adapter.m_mac_address);
            if (existing_entry != m_adapters.end())
            {
                auto& [existing_guid, existing_adapter] = *existing_entry;

                // check for changes to adapter
                if (current_adapter != existing_adapter)
                {
                    m_loggers->m_logger->info("adapter changed - name: {}", current_adapter.m_name);
                    auto changes = adapter_diff(existing_adapter, current_adapter);
                    for (const auto& change : changes)
                    {
                        m_loggers->m_logger->info(std::format(" - {}", change));
                    }

                    if (changes.size())
                    {
                        e_changed(existing_adapter, current_adapter);
                        m_adapters.insert_or_assign(current_adapter.m_mac_address, current_adapter);
                    }
                }
            }
            else
            {
                // new adapter detected
                auto ainfo = adapter_info(current_adapter);
                std::string parameters;
                for (const auto& adapter_paramter : ainfo)
                {
                    if (!parameters.empty())
                    {
                        parameters += ", ";
                    }
                    parameters += adapter_paramter;
                }

                m_adapters.insert({current_adapter.m_mac_address, current_adapter});
                e_new(current_adapter);
            }
        }

        typedef std::map<adapter_identifier_t, adapter_t> adapters_map_t;
        for (adapters_map_t::const_iterator adapter_itr = m_adapters.cbegin(); adapter_itr != m_adapters.cend();)
        {
            auto existing_id = std::find_if(current_adapters.begin(), current_adapters.end(), 
                [&](const adapter_t& val){ return val.m_mac_address == adapter_itr->first; });

            if (existing_id == current_adapters.end())
            {
                // removed adapter detected
                m_loggers->m_logger->info("adapter removed - name: {}", adapter_itr->second.m_name);
                e_removed(adapter_itr->second);
                adapter_itr = m_adapters.erase(adapter_itr);
            }
            else
            {
                adapter_itr = std::next(adapter_itr);
            }
        }
    }

    std::vector<adapter_t> adapter_manager::adapters() const
    {
        std::vector<adapter_t> result;
        for (const auto& adapter : m_adapters | std::views::values)
        {
            result.push_back(adapter);
        }

        return result;
    }

    std::expected<adapter_t, std::string> adapter_manager::find_adapter(const address_t& address) const
    {
        for (const auto& adapter : m_adapters | std::views::values)
        {
            auto existing_adapter = std::find_if(adapter.m_address_list.begin(), adapter.m_address_list.end(), 
                [&](const address_mask_t& val){ return val.first == address; });
            if (existing_adapter != adapter.m_address_list.end())
            {
                return adapter;
            }
        }

        return std::unexpected(std::format("failed to find adapter by address: {}", address.to_string()));
    }

    std::expected<adapter_t, std::string> adapter_manager::find_adapter(const adapter_identifier_t& id) const
    {
        auto itr_adapter = m_adapters.find(id);
        if (itr_adapter != m_adapters.end())
        {
            return itr_adapter->second;
        }

        return std::unexpected(std::format("failed to find adapter by id: {}", boost::lexical_cast<std::string>(id)));
    }
} // !namespace discnet