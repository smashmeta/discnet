/* 
 *
 */

#pragma once

#include <string>
#include <Wbemidl.h>
#include <iphlpapi.h>
#include <discnet/discnet.hpp>

namespace discnet::windows
{
/*
 * WMI (Windows Management Instrumentation) consists of a set of extensions to the 
 * Windows Driver Model that provides an operating system interface through which instrumented
 * components provide information and notification. WMI is Microsoft's implementation of the 
 * Web-Based Enterprise Management (WBEM) and Common Information Model (CIM) standards from the
 * Distributed Management Task Force (DMTF).
 * 
 * Microsoft also provides a command-line interface to WMI called Windows Management Instrumentation
 * Command-line (WMIC).
 */ 

class wbem_consumer;
typedef std::shared_ptr<wbem_consumer> shared_wbem_consumer;

class wbem_consumer
{
    struct impl;
public:
    // no copies of this class allowed
    wbem_consumer(const wbem_consumer& copy) = delete;
    wbem_consumer& operator=(const wbem_consumer& copy) = delete;

    static DISCNET_EXPORT shared_wbem_consumer get_consumer();
    // PS! remember to release enumerator after use.
    DISCNET_EXPORT IEnumWbemClassObject* exec_query(const std::wstring& query);
private:
    wbem_consumer(impl* implementation);
    impl* m_impl;
};

} // !namespace noconn::windows