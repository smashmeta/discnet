/*
 *
 */

#include <iostream>
#include <fmt/format.h>
#include <fmt/color.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/uuid/random_generator.hpp>
#include <discnet_lib/route.hpp>
#include <discnet_lib/adapter_manager.hpp>
#include <discnet_lib/route_manager.hpp>

namespace discnet::test
{
	template<typename ... Ts>
	std::vector<std::byte> make_bytes(Ts&&... args) noexcept
	{
		return { std::byte(std::forward<Ts>(args))... };
	}

	struct adapter_fetcher_mock : public discnet::adapter_fetcher
	{
		MOCK_METHOD(std::vector<discnet::adapter_t>, get_adapters, (), (override));
	};

	struct adapter_manager_callbacks_stub
	{
		virtual void new_adapter(const discnet::adapter_t& adapter) = 0;
		virtual void changed_adapter(const discnet::adapter_t& prev, const discnet::adapter_t& curr) = 0;
		virtual void removed_adapter(const discnet::adapter_t& adapter) = 0;
	};

	struct adapter_manager_callbacks_mock : public adapter_manager_callbacks_stub
	{
		MOCK_METHOD(void, new_adapter, (const discnet::adapter_t&), (override));
		MOCK_METHOD(void, changed_adapter, (const discnet::adapter_t&, const discnet::adapter_t&), (override));
		MOCK_METHOD(void, removed_adapter, (const discnet::adapter_t&), (override));
	};

	typedef discnet::test::adapter_manager_callbacks_mock callback_tester_t;
}

TEST(no_fixture_test, is_direct_node)
{
	auto node_ip = discnet::address_v4_t::from_string("192.169.10.10");
	auto adapter_ip = discnet::address_v4_t::from_string("192.169.10.11");
	auto sender_ip = node_ip;

	discnet::node_identifier_t node{ 1, node_ip };
	discnet::route_identifier direct_route{ node, adapter_ip, sender_ip };

	EXPECT_TRUE(discnet::is_direct_node(direct_route));

	auto indirect_node_ip = discnet::address_v4_t::from_string("102.169.10.12");
	discnet::route_identifier indirect_route{ node, adapter_ip, indirect_node_ip };

	EXPECT_FALSE(discnet::is_direct_node(indirect_route));
}

TEST(no_fixture_test, routes_contains)
{
	auto adapter_ip = discnet::address_v4_t::from_string("192.169.10.11");
	auto sender_1_ip = discnet::address_v4_t::from_string("192.169.10.20");
	auto sender_2_ip = discnet::address_v4_t::from_string("192.169.10.30");
	auto sender_3_ip = discnet::address_v4_t::from_string("192.169.10.40");

	discnet::node_identifier_t node_1{ 1, discnet::address_v4_t::from_string("192.169.10.10") };
	discnet::route_identifier route_1{ node_1, adapter_ip, sender_1_ip };
	discnet::route_identifier route_2{ node_1, adapter_ip, sender_2_ip };
	discnet::route_identifier route_3{ node_1, adapter_ip, sender_3_ip };
	std::vector<discnet::route_identifier> routes = { route_1, route_2 };

	EXPECT_FALSE(discnet::contains(routes, route_3));
	EXPECT_TRUE(discnet::contains(routes, route_2));
}

TEST(no_fixture_test, route_manager_disconnect)
{
	discnet::route_manager_t route_manager;
}

TEST(no_fixture_test, bytes_to_hex_string)
{
	{	// empty list test
		std::vector<std::byte> buffer_empty = {};
		std::string hex_string = discnet::bytes_to_hex_string(buffer_empty);
		EXPECT_EQ(hex_string, "");
	}

	{	// 9-15 test
		std::vector<std::byte> buffer = discnet::test::make_bytes(9, 10, 11, 12, 13, 14, 15);
		std::string hex_string = discnet::bytes_to_hex_string(buffer);
		EXPECT_EQ(hex_string, "09 0A 0B 0C 0D 0E 0F");
	}

	{	// top down test { 0xff, 0xfe, 0xfd, 0x00 }
		std::vector<std::byte> buffer = discnet::test::make_bytes(0xFF, 0xFE, 0xFD, 0x00);
		std::string hex_string = discnet::bytes_to_hex_string(buffer);
		EXPECT_EQ(hex_string, "FF FE FD 00");
	}
}

TEST(no_fixture_test, adapter_manager)
{
	// setting up data
	discnet::adapter_t adapter_1;
	adapter_1.m_guid = boost::uuids::random_generator()();
	adapter_1.m_name = "test_adapter";
	discnet::adapter_t adapter_1_changed_name = adapter_1;
	adapter_1_changed_name.m_name = "test_adapter_changed_name";
	
	std::vector<discnet::adapter_t> adapters = {adapter_1};
	std::vector<discnet::adapter_t> adapters_changed = {adapter_1_changed_name};
	std::vector<discnet::adapter_t> adapters_empty = {};

	{	// making sure that the manager is destroyed (or else gtest will complain about memory leaks)
		auto fetcher = std::make_unique<discnet::test::adapter_fetcher_mock>();
		EXPECT_CALL(*fetcher.get(), get_adapters())
			.Times(3)
			.WillOnce(testing::Return(adapters))
			.WillOnce(testing::Return(adapters_changed))
			.WillOnce(testing::Return(adapters_empty));

		discnet::adapter_manager manager { std::move(fetcher) };
 
		discnet::test::callback_tester_t callbacks_tester;
		manager.e_new.connect(std::bind(&discnet::test::callback_tester_t::new_adapter, &callbacks_tester, std::placeholders::_1));
		manager.e_changed.connect(std::bind(&discnet::test::callback_tester_t::changed_adapter, &callbacks_tester, std::placeholders::_1, std::placeholders::_2));
		manager.e_removed.connect(std::bind(&discnet::test::callback_tester_t::removed_adapter, &callbacks_tester, std::placeholders::_1));
		
		EXPECT_CALL(callbacks_tester, new_adapter(testing::_)).Times(1);
		EXPECT_CALL(callbacks_tester, changed_adapter(testing::_, testing::_)).Times(1);
		EXPECT_CALL(callbacks_tester, removed_adapter(testing::_)).Times(1);

		// first call to update adds test adapter (see WillOnce(test adapter list))
		manager.update();
		// second call to update changes the adapter name (see WillOnce(test change adapter list))
		manager.update();
		// third call to update removes test adapter (see WillOnce(empty adapter list))
		manager.update();
	}
}

int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}