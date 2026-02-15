/*
 *
 */

#pragma once

#include <discnet/network/network_handler.hpp>

namespace discnet::sim::logic
{
    using adapter_identifier_t = uint32_t;

    struct simulator_adapter_entry
    {
        adapter_identifier_t m_identifier;
        adapter_t m_adapter; 
    };

    class simulator_adapter_fetcher : public adapter_fetcher
    {
    public:
        simulator_adapter_fetcher() 
        {
            // nothing for now
        }

        void add_adapter(const adapter_identifier_t identifier, const adapter_t& adapter) 
        { 
            std::lock_guard<std::mutex> lock{m_mutex};
            m_entries.push_back({.m_identifier = identifier, .m_adapter = adapter}); 
        }

        void remove_adapter(const adapter_identifier_t identifier)
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            std::erase_if(m_entries, [=](const auto& entry){ return entry.m_identifier == identifier; });
        }

        std::vector<adapter_t> get_adapters() 
        { 
            std::vector<adapter_t> copy;
            {
                std::lock_guard<std::mutex> lock{m_mutex};
                for (const auto& entry : m_entries)
                {
                    copy.push_back(entry.m_adapter);
                }
            }
            return copy; 
        }

        adapter_identifier_t get_identifier(const discnet::address_t& address) const
        {
            for (const auto& entry : m_entries)
            {
                if (entry.m_adapter.m_address_list.front().first == address)
                {
                    return entry.m_identifier;
                }
            }

            return std::numeric_limits<uint32_t>::max();
        }
    private:
        std::mutex m_mutex;
        std::vector<simulator_adapter_entry> m_entries;
    };
} // !namespace discnet::sim::logic