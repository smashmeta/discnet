/*
 *
 */

#pragma once

#include <expected>
#include <discnet/adapter.hpp>
#include <discnet/application/configuration.hpp>

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

        DISCNET_EXPORT adapter_manager(const discnet::application::configuration_t& configuration, std::shared_ptr<adapter_fetcher> fetcher);
        
        DISCNET_EXPORT void update();

        DISCNET_EXPORT std::vector<adapter_t> adapters() const;

        DISCNET_EXPORT std::expected<adapter_t, std::string> find_adapter(const address_t& address) const;
        DISCNET_EXPORT std::expected<adapter_t, std::string> find_adapter(const adapter_guid_t& id) const;
    protected:
        discnet::application::configuration_t m_configuration;
        discnet::shared_logger m_logger;
        std::shared_ptr<adapter_fetcher> m_fetcher;
        std::map<adapter_guid_t, adapter_t> m_adapters;
    };

    typedef std::shared_ptr<discnet::adapter_manager> shared_adapter_manager;
} // !namespace discnet