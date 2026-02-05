/*
 *
 */

#pragma once

#include <discnet/network/network_handler.hpp>

namespace discnet::sim::logic
{
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
} // !namespace discnet::sim::logic