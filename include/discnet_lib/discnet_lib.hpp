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
    DISCNET_EXPORT std::string bytes_to_hex_string(const std::span<std::byte>& buffer);
} // !namesapce discnet