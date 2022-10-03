/*
 *
 */

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <discnet_lib/route.hpp>

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

	bool contains(const std::span<route_identifier>& routes, const route_identifier& val)
	{
		for (auto& route : routes)
		{
			if (route.m_node.m_id == val.m_node.m_id &&
				route.m_node.m_address == val.m_node.m_address)
			{
				if (route.m_adapter == val.m_adapter &&
					route.m_reporter == val.m_reporter)
				{
					return true;
				}
			}
		}

		return false;
	}
}