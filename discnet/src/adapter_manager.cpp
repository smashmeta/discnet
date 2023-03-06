/*
 *
 */

#include <winsock2.h>
#include <NetCon.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <iphlpapi.h>
#include <vector>
#include <locale>
#include <codecvt>
#include <ranges>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <discnet/adapter_manager.hpp>
#include <whatlog/logger.hpp>

#define _WINSOCK_DEPRECATED_NO_WARNINGS 

namespace discnet
{
    namespace 
    {
        size_t mask_to_cidr(const address_t& mask)
        {
            size_t cidr = 0;
            auto bytes = mask.to_ulong();
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
            result.push_back(std::format("multicast enabled: {}", adapter.m_multicast_enabled));
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

            bool multicast_enabled_changed = (lhs.m_multicast_enabled != rhs.m_multicast_enabled);
            if (multicast_enabled_changed) result.push_back(std::format("multicast enabled: {} => {}", lhs.m_multicast_enabled, rhs.m_multicast_enabled));

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

    adapter_manager::adapter_manager(std::unique_ptr<adapter_fetcher> fetcher)
        : m_fetcher{std::move(fetcher)} 
    {
        // nothing for now
    }

    void adapter_manager::update()
    {
        whatlog::logger log("adapter_manager::update");
        auto current_adapters = m_fetcher->get_adapters();
        for (const adapter_t& current_adapter : current_adapters)
        {
            // found existing entry for adapater
            auto existing_entry = m_adapters.find(current_adapter.m_guid);
            if (existing_entry != m_adapters.end())
            {
                auto& [existing_guid, existing_adapter] = *existing_entry;

                // check for changes to adapter
                if (current_adapter != existing_adapter)
                {
                    log.info("adapter changed - name: {}", current_adapter.m_name);
                    auto changes = adapter_diff(existing_adapter, current_adapter);
                    for (const auto& change : changes)
                    {
                        log.info(" - {}", change);
                    }

                    e_changed(existing_adapter, current_adapter);
                    m_adapters.insert_or_assign(current_adapter.m_guid, current_adapter);
                }
            }
            else
            {
                // new adapter detected
                log.info("new adapter detected.");
                auto ainfo = adapter_info(current_adapter);
                for (const auto& adapter_paramter : ainfo)
                {
                    log.info(" - {}", adapter_paramter);
                }
                m_adapters.insert({current_adapter.m_guid, current_adapter});
                e_new(current_adapter);
            }
        }

        typedef std::map<uuid_t, adapter_t> adapters_map_t;
        for (adapters_map_t::const_iterator adapter_itr = m_adapters.cbegin(); adapter_itr != m_adapters.cend();)
        {
            auto existing_id = std::find_if(current_adapters.begin(), current_adapters.end(), 
                [&](const adapter_t& val){ return val.m_guid == adapter_itr->first; });

            if (existing_id == current_adapters.end())
            {
                // removed adapter detected
                log.info("adapter removed - name: {}", adapter_itr->second.m_name);
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

    std::expected<adapter_t, std::string> adapter_manager::find_adapter(const boost::uuids::uuid& uuid) const
    {
        auto itr_adapter = m_adapters.find(uuid);
        if (itr_adapter != m_adapters.end())
        {
            return itr_adapter->second;
        }

        return std::unexpected(std::format("failed to find adapter by uuid: {}", boost::lexical_cast<std::string>(uuid)));
    }

    windows_adapter_fetcher::windows_adapter_fetcher()
    {
        // nothing for now
    }

    std::vector<std::string> registry_sub_keys(HKEY directory, const std::string& path)
    {
        const size_t max_key_length = 255;
        HKEY reg_key;
        std::vector<std::string> result;

        LSTATUS status = RegOpenKeyEx(directory, TEXT(path.c_str()), 0, KEY_READ, &reg_key);
        if (status == ERROR_SUCCESS)
        {
            TCHAR    achKey[max_key_length];   // buffer for subkey name
            DWORD    cbName;                   // size of name string 
            TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
            DWORD    cchClassName = MAX_PATH;  // size of class string 
            DWORD    cSubKeys = 0;               // number of subkeys 
            DWORD    cbMaxSubKey;              // longest subkey size 
            DWORD    cchMaxClass;              // longest class string 
            DWORD    cValues;              // number of values for key 
            DWORD    cchMaxValue;          // longest value name 
            DWORD    cbMaxValueData;       // longest value data 
            DWORD    cbSecurityDescriptor; // size of security descriptor 
            FILETIME ftLastWriteTime;      // last write time 

            // Get the class name and the value count. 
            status = RegQueryInfoKey(
                reg_key,                 // key handle 
                achClass,                // buffer for class name 
                &cchClassName,           // size of class string 
                NULL,                    // reserved 
                &cSubKeys,               // number of subkeys 
                &cbMaxSubKey,            // longest subkey size 
                &cchMaxClass,            // longest class string 
                &cValues,                // number of values for this key 
                &cchMaxValue,            // longest value name 
                &cbMaxValueData,         // longest value data 
                &cbSecurityDescriptor,   // security descriptor 
                &ftLastWriteTime);       // last write time 

            if (cSubKeys)
            {
                for (DWORD i = 0; i < cSubKeys; i++)
                {
                    cbName = max_key_length;
                    status = RegEnumKeyEx(
                        reg_key,
                        i,
                        achKey,
                        &cbName,
                        NULL,
                        NULL,
                        NULL,
                        &ftLastWriteTime);

                    if (status == ERROR_SUCCESS)
                    {
                        result.push_back(achKey);
                    }
                }
            }
        }

        return result;
    }

    std::expected<std::string, std::string> GetStringRegKey(HKEY directory, const std::string& path, const std::string& strValueName)
    {
        std::expected<std::string, std::string> result;
        HKEY reg_key;
        LSTATUS status = RegOpenKeyEx(directory, TEXT(path.c_str()), 0, KEY_READ, &reg_key);
        if (status == ERROR_SUCCESS)
        {
            CHAR szBuffer[512];
            DWORD dwBufferSize = sizeof(szBuffer);
            ULONG nError;

            nError = RegQueryValueEx(reg_key, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
            if (status == ERROR_SUCCESS)
            {
                std::string sub_key_name = szBuffer;
                if (sub_key_name != "Descriptions")
                {
                    result = sub_key_name;
                }
                
            }
        }
        else
        {
            result = std::unexpected(std::format("failed to load registry path {}", path));
        }

        return result;
    }

    std::string get_adapter_guid(const std::string& adapter_name)
    {
        const std::string adapters_path = "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}";

        std::vector<std::string> adapter_keys = registry_sub_keys(HKEY_LOCAL_MACHINE, adapters_path);
        for (const std::string& adapter : adapter_keys)
        {
            std::string adapter_path = adapters_path + "\\" + adapter + "\\Connection";
            auto current_adapter_name = GetStringRegKey(HKEY_LOCAL_MACHINE, adapter_path, "Name");
            if (current_adapter_name && current_adapter_name.value() == adapter_name)
            {
                return adapter;
            }
        }

        return "";
    }

    std::vector<adapter_t> windows_adapter_fetcher::get_adapters()
    {
        whatlog::logger log("list_adapter_ip_addresses");
        ULONG  outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
        PIP_ADAPTER_ADDRESSES  pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        std::vector<adapter_t> result;

        // default to unspecified address family (both)
        ULONG family = AF_INET; // ipv4 only (AF_UNSPEC for both)

        // Set the flags to pass to GetAdaptersAddresses
        ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;


        // Make an initial call to GetAdaptersAddresses to get the 
        // size needed into the outBufLen variable
        if (GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
        {
            free(pAddresses);
            pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        }

        if (pAddresses == NULL)
        {
            log.error("Memory allocation failed for IP_ADAPTER_ADDRESSES struct.");
            return result;
        }

        PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
        DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
        if (dwRetVal == NO_ERROR)
        {
            // If successful, output some information from the data we received
            pCurrAddresses = pAddresses;
            while (pCurrAddresses)
            {
                std::string mac_address = discnet::bytes_to_hex_string(
                    std::span<discnet::byte_t>(pCurrAddresses->PhysicalAddress, pCurrAddresses->PhysicalAddressLength));
                adapter_t adapter;
                adapter.m_mac_address = mac_address;
                adapter.m_index = static_cast<uint8_t>(pCurrAddresses->IfIndex);
                adapter.m_name = converter.to_bytes(std::wstring(pCurrAddresses->FriendlyName));
                adapter.m_description = converter.to_bytes(std::wstring(pCurrAddresses->Description));
                adapter.m_enabled = pCurrAddresses->OperStatus == IfOperStatusUp;
                adapter.m_gateway = discnet::address_t::any();
                adapter.m_mtu = pCurrAddresses->Mtu;

                IFTYPE if_type = pCurrAddresses->IfType;
                bool valid_if_type = 
                    if_type == IF_TYPE_ETHERNET_CSMACD ||
                    if_type == IF_TYPE_PPP ||
                    if_type == IF_TYPE_IEEE80211;

                bool virtual_adapters_filter = boost::algorithm::starts_with(adapter.m_name, "Local Area Connection*");

                if (!valid_if_type || virtual_adapters_filter)
                {
                    pCurrAddresses = pCurrAddresses->Next;
                    continue;
                }

                bool multicast_available = pCurrAddresses->FirstMulticastAddress != nullptr && 
                    pCurrAddresses->FirstMulticastAddress->Next != nullptr;        
                adapter.m_multicast_enabled = multicast_available;

                std::string guid_str = get_adapter_guid(adapter.m_name);
                if (guid_str.empty() || guid_str.size() != 38)
                {
                    pCurrAddresses = pCurrAddresses->Next;
                    continue;
                }

                std::string guid_str_sanitized = guid_str.substr(1, guid_str.size() - 2);
                adapter.m_guid = boost::lexical_cast<boost::uuids::uuid>(guid_str_sanitized);

                // log.info(" - name: {}.", adapter.m_name);
                // log.info(" - index: {}.", adapter.m_index);
                // log.info(" - mac: {}.", adapter.m_mac_address);
                // log.info(" - desc: {}.", adapter.m_description);
                // log.info(" - enabled: {}.", adapter.m_enabled);
                // log.info(" - multicast-enabled: {}.", adapter.m_multicast_enabled);
                // log.info(" - guid: {}.", boost::lexical_cast<std::string>(adapter.m_guid));

                PIP_ADAPTER_GATEWAY_ADDRESS gateway_address = pCurrAddresses->FirstGatewayAddress;
                for (size_t index = 0; gateway_address != nullptr; ++index)
                {
                    if (gateway_address->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        discnet::address_t address;
                        bool valid_ip_address = true;

                        ULONG buffer_size = 256;
                        std::array<wchar_t, 256> buffer;
                        long ip_to_string_cast_result = WSAAddressToStringW(gateway_address->Address.lpSockaddr, gateway_address->Address.iSockaddrLength, nullptr, const_cast<wchar_t*>(buffer.data()), &buffer_size);
                        if (ip_to_string_cast_result == NO_ERROR)
                        {
                            std::wstring address_wstr = buffer.data();
                            address = discnet::address_t::from_string(converter.to_bytes(address_wstr));
                            // log.info("{} - gateway: {}.", index, address.to_string());
                        }
                        else
                        {
                            log.error("failed to cast gateway address struct to string.");
                            valid_ip_address = false;
                        }

                        if (valid_ip_address)
                        {
                            adapter.m_gateway = address;
                        }
                    }
                    else
                    {
                        log.warning("gateway_address->Address.lpSockaddr->sa_family = {}.", gateway_address->Address.lpSockaddr->sa_family);
                    }

                    gateway_address = gateway_address->Next;
                }

                PIP_ADAPTER_UNICAST_ADDRESS unicast_address = pCurrAddresses->FirstUnicastAddress;
                for (size_t index = 0; unicast_address != nullptr; ++index)
                {
                    if (unicast_address->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        discnet::address_mask_t address;
                        bool valid_ip_address = true;

                        ULONG buffer_size = 256;
                        std::array<wchar_t, 256> buffer;
                        long ip_to_string_cast_result = WSAAddressToStringW(unicast_address->Address.lpSockaddr, unicast_address->Address.iSockaddrLength, nullptr, const_cast<wchar_t*>(buffer.data()), &buffer_size);
                        if (ip_to_string_cast_result == NO_ERROR)
                        {
                            std::wstring address_wstr = buffer.data();
                            address.first = discnet::address_t::from_string(converter.to_bytes(address_wstr));
                            // log.info("{} - address: {}.", index, address.first.to_string());
                        }
                        else
                        {
                            log.error("failed to cast unicast address struct to string.");
                            valid_ip_address = false;
                        }

                        unsigned long netmask = 0;
                        if (ConvertLengthToIpv4Mask(unicast_address->OnLinkPrefixLength, &netmask) == NO_ERROR)
                        {
                            in_addr netmask_address;
                            netmask_address.S_un.S_addr = netmask;
                            char netmask_str[INET_ADDRSTRLEN];
                            inet_ntop(AF_INET, &(netmask_address.S_un.S_addr), netmask_str, INET_ADDRSTRLEN);
                            address.second = discnet::address_t::from_string(netmask_str);

                            // log.info("{} - mask: {}.", index, address.second.to_string());
                        }
                        else
                        {
                            log.error("failed to convert netmask length \"{}\" to ip address.", unicast_address->OnLinkPrefixLength);
                            valid_ip_address = false;
                        }

                        if (valid_ip_address)
                        {
                            adapter.m_address_list.push_back(address);
                            //log.info("{} - addr: {}, mask: {}.", index, address.first.to_string(), address.second.to_string());
                        }
                    }
                    else if (unicast_address->Address.lpSockaddr->sa_family == AF_INET6)
                    {
                        // ignore for now
                    }
                    else
                    {
                        log.warning("unicast_address->Address.lpSockaddr->sa_family = {}.", unicast_address->Address.lpSockaddr->sa_family);
                    }

                    unicast_address = unicast_address->Next;
                }

                PIP_ADAPTER_DNS_SERVER_ADDRESS dns_address = pCurrAddresses->FirstDnsServerAddress;
                for (size_t index = 0; dns_address != nullptr; ++index)
                {
                    if (dns_address->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        discnet::address_t address;
                        bool valid_ip_address = true;

                        ULONG buffer_size = 256;
                        std::array<wchar_t, 256> buffer;
                        long ip_to_string_cast_result = WSAAddressToStringW(dns_address->Address.lpSockaddr, dns_address->Address.iSockaddrLength, nullptr, const_cast<wchar_t*>(buffer.data()), &buffer_size);
                        if (ip_to_string_cast_result == NO_ERROR)
                        {
                            std::wstring address_wstr = buffer.data();
                            address = discnet::address_t::from_string(converter.to_bytes(address_wstr));
                            // log.info("{} - dns: {}.", index, address.to_string());
                        }
                        else
                        {
                            log.error("failed to cast dns address struct to string.");
                            valid_ip_address = false;
                        }
                    }
                    else
                    {
                        log.warning("dns_address->Address.lpSockaddr->sa_family = {}.", dns_address->Address.lpSockaddr->sa_family);
                    }

                    dns_address = dns_address->Next;
                }

                PIP_ADAPTER_MULTICAST_ADDRESS multicast_address = pCurrAddresses->FirstMulticastAddress;
                for (size_t index = 0; multicast_address != nullptr; ++index)
                {
                    if (multicast_address->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        discnet::address_t address;
                        bool valid_ip_address = true;

                        ULONG buffer_size = 256;
                        std::array<wchar_t, 256> buffer;
                        long ip_to_string_cast_result = WSAAddressToStringW(multicast_address->Address.lpSockaddr, multicast_address->Address.iSockaddrLength, nullptr, const_cast<wchar_t*>(buffer.data()), &buffer_size);
                        if (ip_to_string_cast_result == NO_ERROR)
                        {
                            std::wstring address_wstr = buffer.data();
                            address = discnet::address_t::from_string(converter.to_bytes(address_wstr));
                            // log.info("{} - multicast-address: {}.", index, address.to_string());
                        }
                        else
                        {
                            log.error("failed to cast dns address struct to string.");
                            valid_ip_address = false;
                        }
                    }
                    else
                    {
                        log.warning("multicast_address->Address.lpSockaddr->sa_family = {}.", multicast_address->Address.lpSockaddr->sa_family);
                    }

                    multicast_address = multicast_address->Next;
                }

                result.push_back(adapter);

                pCurrAddresses = pCurrAddresses->Next;
            }
        }
        else
        {
            log.error("Call to GetAdaptersAddresses failed with error: {}.", dwRetVal);
            if (dwRetVal == ERROR_NO_DATA)
            {
                log.error("No addresses were found for the requested parameters.");
            }
            else
            {
                LPVOID lpMsgBuf = NULL;
                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL))
                {
                    log.error("error: {}", std::string((LPTSTR)lpMsgBuf));
                    LocalFree(lpMsgBuf);

                    free(pAddresses);
                    exit(1);
                }
            }
        }

        free(pAddresses);
        return result;
    }
} // !namespace discnet