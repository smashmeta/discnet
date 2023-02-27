/*
 *
 */

#pragma once

#include <discnet/adapter.hpp>
#include <discnet/windows/wbem_consumer.hpp>

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
        DISCNET_EXPORT windows_adapter_fetcher(discnet::windows::shared_wbem_consumer consumer);
        DISCNET_EXPORT std::vector<adapter_t> get_adapters() override;
    private:
        discnet::windows::shared_wbem_consumer m_consumer;
    };

    class adapter_manager_t
    {
    public:
        boost::signals2::signal<void(const adapter_t&)> e_new;
        boost::signals2::signal<void(const adapter_t& prev, const adapter_t& curr)> e_changed;
        boost::signals2::signal<void(const adapter_t&)> e_removed;

        DISCNET_EXPORT adapter_manager_t(std::unique_ptr<adapter_fetcher> fetcher);
        DISCNET_EXPORT bool is_equal(const adapter_t& lhs, const adapter_t& rhs);
        DISCNET_EXPORT void update();

        DISCNET_EXPORT adapter_t find_adapter(const address_t& address) const;
    protected:
        std::unique_ptr<adapter_fetcher> m_fetcher;
        std::map<uuid_t, adapter_t> m_adapters;
    };
} // !namespace discnet