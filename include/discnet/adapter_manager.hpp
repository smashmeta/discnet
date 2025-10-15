/*
 *
 */

#pragma once

#include <expected>
#include <discnet/adapter.hpp>

namespace discnet
{
    class adapter_fetcher
    {
    public:
        virtual ~adapter_fetcher() {}
        virtual std::vector<adapter_t> get_adapters() = 0; 
    };
    
    class adapter_manager
    {
    public:
        boost::signals2::signal<void(const adapter_t&)> e_new;
        boost::signals2::signal<void(const adapter_t& prev, const adapter_t& curr)> e_changed;
        boost::signals2::signal<void(const adapter_t&)> e_removed;

        DISCNET_EXPORT adapter_manager(std::unique_ptr<adapter_fetcher> fetcher);
        
        DISCNET_EXPORT void update();

        DISCNET_EXPORT void update_multicast_present(const adapter_identifier_t& uuid, bool present);

        DISCNET_EXPORT std::vector<adapter_t> adapters() const;

        DISCNET_EXPORT std::expected<adapter_t, std::string> find_adapter(const address_t& address) const;
        DISCNET_EXPORT std::expected<adapter_t, std::string> find_adapter(const adapter_identifier_t& uuid) const;
    protected:
        std::unique_ptr<adapter_fetcher> m_fetcher;
        std::map<adapter_identifier_t, adapter_t> m_adapters;
    };

    typedef std::shared_ptr<discnet::adapter_manager> shared_adapter_manager;
} // !namespace discnet