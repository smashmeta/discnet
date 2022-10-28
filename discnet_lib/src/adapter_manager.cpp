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
#include <fmt/format.h>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <discnet_lib/adapter_manager.hpp>
#include <whatlog/logger.hpp>

namespace discnet
{
    adapter_manager_t::adapter_manager_t(std::unique_ptr<adapter_fetcher> fetcher)
        : m_fetcher{std::move(fetcher)} 
    {
        // nothing for now
    }

    bool adapter_manager_t::is_equal(const adapter_t& lhs, const adapter_t& rhs)
    {
        return  lhs.m_guid == rhs.m_guid &&
                lhs.m_mac_address == rhs.m_mac_address &&
                lhs.m_name == rhs.m_name && 
                lhs.m_index == rhs.m_index &&
                lhs.m_description == rhs.m_description &&
                lhs.m_enabled == rhs.m_enabled &&
                lhs.m_address_list == rhs.m_address_list;
    }

    void adapter_manager_t::update()
    {
        auto current_adapters = m_fetcher->get_adapters();
        for (const adapter_t& adapter : current_adapters)
        {
            // found existing entry for adapater
            auto existing_id = m_adapters.find(adapter.m_guid);
            if (existing_id != m_adapters.end())
            {
                // check for changes to adapter
                if (!is_equal(adapter, existing_id->second))
                {
                    e_changed(existing_id->second, adapter);
                }
            }
            else
            {
                // new adapter detected
                m_adapters.insert({adapter.m_guid, adapter});
                e_new(adapter);
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
                e_removed(adapter_itr->second);
                adapter_itr = m_adapters.erase(adapter_itr);
            }
            else
            {
                adapter_itr = std::next(adapter_itr);
            }
        }
    }

    adapter_t adapter_manager_t::find_adapter(const address_v4_t& address) const
    {
        for (const auto& adapter : m_adapters | std::views::values)
        {
            auto existing_adapter = std::find_if(adapter.m_address_list.begin(), adapter.m_address_list.end(), 
                [&](const address_mask_v4_t& val){ return val.first == address; });
            if (existing_adapter != adapter.m_address_list.end())
            {
                return adapter;
            }
        }

        return {};
    }

    windows_adapter_fetcher::windows_adapter_fetcher(discnet::windows::shared_wbem_consumer consumer)
        : m_consumer(consumer)
    {
        // nothing for now
    }

    std::vector<adapter_t> windows_adapter_fetcher::get_adapters()
    {
        whatlog::logger log("get_network_adapters");

        std::vector<adapter_t> result;
        std::wstring query = L"SELECT * FROM Win32_NetworkAdapter";
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        IEnumWbemClassObject* pEnumerator = m_consumer->exec_query(query);
        if (pEnumerator != nullptr)
        {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;

            while (pEnumerator)
            {
                pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn)
                {
                    break;
                }

                VARIANT vtProp;
                HRESULT hr_guid = pclsObj->Get(L"GUID", 0, &vtProp, 0, 0);
                // only list adapters with a valid GUID
                if (vtProp.pbstrVal != nullptr)
                {
                    std::wstring guid = vtProp.bstrVal;

                    HRESULT hr_name = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
                    std::wstring name = vtProp.bstrVal;

                    HRESULT hr_desc = pclsObj->Get(L"Description", 0, &vtProp, 0, 0);
                    std::wstring desc = vtProp.bstrVal;

                    HRESULT hr_adapter_type = pclsObj->Get(L"AdapterType", 0, &vtProp, 0, 0);
                    std::wstring adapter_type = L"[UNAVAILABLE]";
                    if (vtProp.pbstrVal != nullptr)
                    {
                        adapter_type = vtProp.bstrVal;
                    }

                    HRESULT hr_connection_name = pclsObj->Get(L"NetConnectionID", 0, &vtProp, 0, 0);
                    std::wstring connection_name = vtProp.bstrVal;

                    HRESULT hr_interface_index = pclsObj->Get(L"InterfaceIndex", 0, &vtProp, 0, 0);
                    int index = vtProp.intVal;

                    HRESULT hr_enabled = pclsObj->Get(L"NetEnabled", 0, &vtProp, 0, 0);
                    bool enabled = vtProp.boolVal;

                    HRESULT hr_mac_address = pclsObj->Get(L"MACAddress", 0, &vtProp, 0, 0);
                    std::wstring mac_address = vtProp.bstrVal;

                    if (SUCCEEDED(hr_guid) &&
                        SUCCEEDED(hr_name) &&
                        SUCCEEDED(hr_desc) &&
                        SUCCEEDED(hr_adapter_type) &&
                        SUCCEEDED(hr_connection_name) &&
                        SUCCEEDED(hr_interface_index) &&
                        SUCCEEDED(hr_enabled) && 
                        SUCCEEDED(hr_mac_address))
                    {

                        adapter_t adapter;
                        adapter.m_guid = boost::lexical_cast<discnet::uuid_t>(converter.to_bytes(guid));
                        adapter.m_name = converter.to_bytes(connection_name); 
                        adapter.m_description = converter.to_bytes(desc); 
                        adapter.m_index = static_cast<uint8_t>(index);
                        adapter.m_enabled = enabled;
                        adapter.m_mac_address = converter.to_bytes(mac_address);

                        result.emplace_back(std::move(adapter));
                    }
                }

                VariantClear(&vtProp);
                pclsObj->Release();
            }

            pEnumerator->Release();
        }
        else
        {
            log.error(fmt::format("failed to execute query \"{}\".", converter.to_bytes(query)));
        }

        return result;
    }
} // !namespace discnet