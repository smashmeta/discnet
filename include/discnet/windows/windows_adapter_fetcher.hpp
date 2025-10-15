/*
 *
 */

#pragma once

#include <expected>
#include <discnet/adapter_manager.hpp>

namespace discnet
{
    class windows_adapter_fetcher : public adapter_fetcher
    {
    public:
        DISCNET_EXPORT windows_adapter_fetcher();
        DISCNET_EXPORT std::vector<adapter_t> get_adapters() override;
    };
} // !namespace discnet