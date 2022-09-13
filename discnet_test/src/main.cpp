/*
 *
 */

#include <iostream>
#include <fmt/format.h>
#include <fmt/color.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "discnet_lib/discnet_lib.hpp"

namespace discnet::test
{

}

TEST(no_fixture_test, is_direct_node)
{
	auto node_ip = discnet::address_v4_t::from_string("192.169.10.10");
	auto adapter_ip = discnet::address_v4_t::from_string("192.169.10.11");
	auto sender_ip = node_ip;

	discnet::node_identifier node{ 1, node_ip };
	discnet::route_identifier direct_route{ node, adapter_ip, sender_ip };

	EXPECT_TRUE(discnet::is_direct_node(direct_route));

	auto indirect_node_ip = discnet::address_v4_t::from_string("102.169.10.12");
	discnet::route_identifier indirect_route{ node, adapter_ip, indirect_node_ip };

	EXPECT_FALSE(discnet::is_direct_node(indirect_route));
}

TEST(no_fixture_test, is_unique_node)
{
	auto adapter_ip = discnet::address_v4_t::from_string("192.169.10.11");
	auto sender_1_ip = discnet::address_v4_t::from_string("192.169.10.20");
	auto sender_2_ip = discnet::address_v4_t::from_string("192.169.10.30");
	auto sender_3_ip = discnet::address_v4_t::from_string("192.169.10.40");

	discnet::node_identifier node_1{ 1, discnet::address_v4_t::from_string("192.169.10.10") };
	discnet::route_identifier route_1{ node_1, adapter_ip, sender_1_ip };
	discnet::route_identifier route_2{ node_1, adapter_ip, sender_2_ip };
	discnet::route_identifier route_3{ node_1, adapter_ip, sender_3_ip };
	std::vector<discnet::route_identifier> routes = { route_1, route_2 };

	EXPECT_TRUE(discnet::is_unique_route(routes, route_3));
	EXPECT_FALSE(discnet::is_unique_route(routes, route_2));
}

TEST(no_fixture_test, bytes_to_hex_string)
{
	{	// empty list test
		std::vector<std::byte> buffer_empty = {};
		std::string hex_string = discnet::bytes_to_hex_string(buffer_empty);
		EXPECT_EQ(hex_string, "");
	}

	{	// 9-15 test
		std::vector<std::byte> buffer = {
			std::byte{9}, std::byte{10}, std::byte{11}, std::byte{12},
			std::byte{13}, std::byte{14}, std::byte{15} 
		};

		std::string hex_string = discnet::bytes_to_hex_string(buffer);
		EXPECT_EQ(hex_string, "09 0A 0B 0C 0D 0E 0F");
	}

	{	// top down test { 0xff, 0xfe, ..., 0x00 }
		std::vector<std::byte> buffer = {
			std::byte{0xFF}, std::byte{0xFE}, std::byte{0xFD}, std::byte{0x00}
		};

		std::string hex_string = discnet::bytes_to_hex_string(buffer);
		EXPECT_EQ(hex_string, "FF FE FD 00");
	}
}

int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}