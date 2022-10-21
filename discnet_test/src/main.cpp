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
#include <discnet_lib/discovery_message.hpp>
#include <discnet_lib/network/buffer.hpp>

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

TEST(no_fixture_test, adapter_manager__update)
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

TEST(no_fixture_test, adapter_manager__find_adapter)
{
	using ipv4 = boost::asio::ip::address_v4;

	// setting up data
	discnet::adapter_t adapter_1;
	adapter_1.m_guid = boost::uuids::random_generator()();
	adapter_1.m_index = 0;
	adapter_1.m_name = "test_adapter1";
	adapter_1.m_description = "description_adapter_1";
	adapter_1.m_enabled = false;
	adapter_1.m_gateway = ipv4::from_string("0.0.0.0");
	adapter_1.m_address_list = { {ipv4::from_string("192.200.1.3"), ipv4::from_string("255.255.255.0")},
								 {ipv4::from_string("192.169.40.130"), ipv4::from_string("255.255.255.0")} };
	discnet::adapter_t adapter_2;
	adapter_2.m_guid = boost::uuids::random_generator()();
	adapter_2.m_index = 1;
	adapter_2.m_name = "test_adapter_2";
	adapter_2.m_description = "description_adapter_2";
	adapter_2.m_gateway = ipv4::from_string("0.0.0.0");
	adapter_1.m_enabled = true;
	adapter_2.m_address_list = { {ipv4::from_string("10.0.0.1"), ipv4::from_string("255.255.255.0")}};

	std::vector<discnet::adapter_t> adapters = {adapter_1, adapter_2};

	{	// making sure that the manager is destroyed (or else gtest will complain about memory leaks)
		auto fetcher = std::make_unique<discnet::test::adapter_fetcher_mock>();
		EXPECT_CALL(*fetcher.get(), get_adapters()).Times(1).WillOnce(testing::Return(adapters));
		discnet::adapter_manager manager { std::move(fetcher) };
		manager.update();

		auto adapter_valid_10 = manager.find_adapter(ipv4::from_string("10.0.0.1"));
		EXPECT_EQ(adapter_valid_10.m_guid, adapter_2.m_guid);
		auto adapter_valid_192 = manager.find_adapter(ipv4::from_string("192.200.1.3"));
		EXPECT_EQ(adapter_valid_192.m_name, adapter_1.m_name);
		auto adapter_failed_not_contained = manager.find_adapter(ipv4::from_string("10.11.12.13"));
		EXPECT_EQ(adapter_failed_not_contained.m_guid, boost::uuids::nil_uuid());
	}
}

TEST(no_fixture_test, buffer_t__packet)
{
	using ipv4 = boost::asio::ip::address_v4;

	discnet::discovery_message_t message;
	message.m_id = 1024;
	message.m_nodes = { discnet::node_identifier_t{ 1025, ipv4::from_string("192.200.1.1") } };

	discnet::network::buffer_t buffer(1024);
	discnet::message_list_t messages = { message };
	discnet::packet_codec_t::encode(buffer, messages);

	std::string output = discnet::bytes_to_hex_string(buffer.data());

	// packet: size + message count :	{ 00 00 00 28, 00 01 } = { 40, 1 }
	// header: size + type :			{ 00 00 00 0C, 00 01 } = { 12, 1 }
	// discovery id + element count :	{ 04 00 00 00, 00 01 } = { 1024, 1 }
	// element 1 :						{ 04 01, C0 C8 01 01 } = { 1025, 192.200.1.1 }
	// checksum :						{ 2A 9C 1C A8 } = 4-bytes
	EXPECT_EQ(output, 
		"00 00 00 1C 00 01 "
		"00 00 00 0C 00 01 "
		"04 00 00 00 00 01 "
		"04 01 C0 C8 01 01 "
		"2A 9C 1C A8");
}

int main(int arguments_count, char** arguments_vector) 
{
	testing::InitGoogleTest(&arguments_count, arguments_vector);
	return RUN_ALL_TESTS();
}