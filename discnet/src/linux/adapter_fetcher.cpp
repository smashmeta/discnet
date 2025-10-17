/*
 *
 */

#include <discnet/linux/adapter_fetcher.hpp>
#include <ifaddrs.h>


namespace discnet
{
    linux_adapter_fetcher::linux_adapter_fetcher()
    {
        // nothing for now
    }
    
    std::vector<adapter_t> linux_adapter_fetcher::get_adapters()
    {
        

        return {};
    }
} // !namespace discnet