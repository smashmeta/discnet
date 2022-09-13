/*
 *
 */

#include <sstream>
#include <boost/algorithm/string.hpp>
#include "discnet_lib/discnet_lib.hpp"

namespace discnet
{
	bool is_route_online(const route_t& route)
	{
		if (route.m_persistent)
		{
			return true;
		}

		auto current_time = std::chrono::system_clock::now();
		auto breakof_time = current_time - std::chrono::seconds(90);
		if (route.m_last_tdp > current_time ||
			route.m_last_data_message > current_time)
		{
			return true;
		}

		return false;
	}

	bool is_direct_node(const route_identifier& route)
	{
		if (route.m_node.m_address == route.m_reporter)
		{
			return true;
		}

		return false;
	}

	bool is_unique_route(const std::span<route_identifier>& routes, const route_identifier& val)
	{
		for (auto& route : routes)
		{
			if (route.m_node.m_id == val.m_node.m_id &&
				route.m_node.m_address == val.m_node.m_address)
			{
				if (route.m_adapter == val.m_adapter &&
					route.m_reporter == val.m_reporter)
				{
					return false;
				}
			}
		}

		return true;
	}

    std::string bytes_to_hex_string(const std::span<std::byte>& buffer)
    {
		std::stringstream result;

		for (size_t i = 0; i < buffer.size(); ++i)
		{
			const std::byte& byte = buffer[i];
			if (i > 0)
			{
				result << " ";
			}

			const int first_nibble = ((unsigned char)byte >> 4) & 0xf;
			const int second_nibble = (unsigned char)byte & 0xf;
			result << std::hex << first_nibble << std::hex << second_nibble;
		}

		return boost::to_upper_copy(result.str());
    }
} // !namespace discnet