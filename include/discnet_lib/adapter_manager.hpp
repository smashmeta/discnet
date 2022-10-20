/*
 *
 */

#pragma once

#include <discnet_lib/adapter.hpp>
#include <discnet_lib/windows/wbem_consumer.hpp>

namespace discnet
{
    class adapter_fetcher
    {
    public:
        virtual ~adapter_fetcher() {}
        virtual std::vector<adapter_t> get_adapters() = 0; 
    };

    struct windows_adapter_fetcher : public adapter_fetcher
    {
        windows_adapter_fetcher(discnet::windows::shared_wbem_consumer consumer);
        std::vector<adapter_t> get_adapters() override;
    private:
        discnet::windows::shared_wbem_consumer m_consumer;
    };

    class adapter_manager
    {
    public:
        boost::signals2::signal<void(const adapter_t&)> e_new;
        boost::signals2::signal<void(const adapter_t& prev, const adapter_t& curr)> e_changed;
        boost::signals2::signal<void(const adapter_t&)> e_removed;

        adapter_manager(std::unique_ptr<adapter_fetcher> fetcher);
        bool is_equal(const adapter_t& lhs, const adapter_t& rhs);
        void update();

        adapter_t find_adapter(const address_v4_t& address) const; 
    protected:
        std::unique_ptr<adapter_fetcher> m_fetcher;
        std::map<uuid_t, adapter_t> m_adapters;
    };
} // !namespace discnet