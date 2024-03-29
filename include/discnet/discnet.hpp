/*
 *
 */

#pragma once

#include <string>
#include <span>
#include <cstddef>
#include <chrono>
#include <map>
#include <boost/signals2.hpp>
#include <memory>

#ifdef DISCNET_DLL
#  define DISCNET_EXPORT __declspec(dllexport)
#else
#  define DISCNET_EXPORT __declspec(dllimport)
#endif

namespace discnet
{
    typedef uint8_t byte_t;

    DISCNET_EXPORT std::string bytes_to_hex_string(const std::span<const std::byte>& buffer);
    DISCNET_EXPORT std::string bytes_to_hex_string(const std::span<const discnet::byte_t>& buffer);
    DISCNET_EXPORT std::string sha256_file(const std::string& filename);
} // !namesapce discnet