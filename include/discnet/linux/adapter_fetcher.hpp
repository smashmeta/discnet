/*
 *
 */

#pragma once

#include <discnet/adapter_manager.hpp>

namespace discnet
{
    class linux_adapter_fetcher : public adapter_fetcher
    {
    public:
        DISCNET_EXPORT linux_adapter_fetcher();
        DISCNET_EXPORT std::vector<adapter_t> get_adapters() override;
    };
} // !namespace discnet