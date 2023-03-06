/*
 *
 */

#include <numeric>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <discnet/route.hpp>

namespace discnet
{
	bool is_shorter_route(const route_t& lhs, const route_t& rhs)
	{
		auto lhs_length = std::accumulate(lhs.m_status.m_jumps.begin(), lhs.m_status.m_jumps.end(), 0);
		auto rhs_length = std::accumulate(rhs.m_status.m_jumps.begin(), rhs.m_status.m_jumps.end(), 0);
		return lhs_length < rhs_length;
	}

    bool is_route_online(const route_t& route, const time_point_t& current_time)
	{
		if (route.m_status.m_persistent)
		{
			return true;
		}

		auto timeout_limit = current_time - std::chrono::seconds(90);
		if (route.m_last_discovery > timeout_limit ||
			route.m_last_data_message > timeout_limit)
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