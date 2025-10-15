/* 
 *
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/uuid/random_generator.hpp>
#include <discnet/route_manager.hpp>

namespace discnet::test
{
    struct adapter_fetcher_mock : public discnet::adapter_fetcher
    {
        MOCK_METHOD(std::vector<discnet::adapter_t>, get_adapters, (), (override));
    };
} // ! namespace discnet::test

class route_manager_fixture : public ::testing::Test
{
protected:
    using ipv4 = discnet::address_t;
public:
    virtual void SetUp() override
    {
        // setting up data
        m_adapter_1.m_index = 1;
        m_adapter_1.m_enabled = true;
        m_adapter_1.m_multicast_enabled = true;
        m_adapter_1.m_guid = boost::uuids::random_generator()();
        m_adapter_1.m_name = "adapter_1";
        m_adapter_1.m_address_list = { {boost::asio::ip::make_address_v4("192.200.10.1"), boost::asio::ip::make_address_v4("255.255.255.0")} };

        m_adapter_2.m_index = 2;
        m_adapter_2.m_enabled = true;
        m_adapter_2.m_multicast_enabled = true;
        m_adapter_2.m_guid = boost::uuids::random_generator()();
        m_adapter_2.m_name = "adapter_2";
        m_adapter_2.m_address_list = { {boost::asio::ip::make_address_v4("192.200.20.2"), boost::asio::ip::make_address_v4("255.255.255.0")} };

        m_adapter_3.m_index = 3;
        m_adapter_3.m_enabled = true;
        m_adapter_3.m_multicast_enabled = true;
        m_adapter_3.m_guid = boost::uuids::random_generator()();
        m_adapter_3.m_name = "adapter_3";
        m_adapter_3.m_address_list = { {boost::asio::ip::make_address_v4("192.200.30.3"), boost::asio::ip::make_address_v4("255.255.255.0")} };

        std::vector<discnet::adapter_t> adapters = {m_adapter_1, m_adapter_2, m_adapter_3};
        auto fetcher = std::make_unique<discnet::test::adapter_fetcher_mock>();
        EXPECT_CALL(*fetcher.get(), get_adapters())
            .WillRepeatedly(testing::Return(adapters));

        m_adapter_manager = std::make_shared<discnet::adapter_manager>(std::move(fetcher));
        m_route_manager = std::make_shared<discnet::route_manager>(m_adapter_manager);

        m_adapter_manager->update();
    }
protected:
    discnet::address_t first_ip(const discnet::adapter_t& adapter)
    {
        return adapter.m_address_list.front().first;
    }

    discnet::adapter_t m_adapter_1;
    discnet::adapter_t m_adapter_2;
    discnet::adapter_t m_adapter_3;

    discnet::shared_adapter_manager m_adapter_manager;
    discnet::shared_route_manager m_route_manager;
};

TEST_F(route_manager_fixture, process_single_discovery_message)
{
    auto time = discnet::time_point_t::clock::now();

    discnet::network::network_info_t network_info;
    network_info.m_reception_time = time;
    network_info.m_sender = boost::asio::ip::make_address_v4("192.200.10.2");
    network_info.m_adapter = first_ip(m_adapter_1);
    network_info.m_receiver = boost::asio::ip::make_address_v4("234.5.6.7");

    auto rcv_adapter = m_adapter_manager->find_adapter(network_info.m_adapter);
    ASSERT_TRUE(rcv_adapter);

    discnet::network::messages::discovery_message_t message {.m_identifier = 2};
    message.m_nodes.push_back({.m_identifier = 3, .m_address = boost::asio::ip::make_address_v4("192.200.10.3"), .m_jumps {256}});
    m_route_manager->process(message, network_info);
    m_route_manager->update(time);

    auto routes = m_route_manager->find_routes_on_adapter(rcv_adapter.value().m_guid);
    ASSERT_EQ(routes.size(), 2);
    // first: direct route
    EXPECT_EQ(routes[0].m_identifier.m_node.m_id, 2);
    EXPECT_EQ(routes[0].m_identifier.m_node.m_address, network_info.m_sender);
    EXPECT_EQ(routes[0].m_identifier.m_adapter, network_info.m_adapter);
    EXPECT_EQ(routes[0].m_identifier.m_reporter, network_info.m_sender); // direct node
    EXPECT_EQ(routes[0].m_status.m_online, true);
    ASSERT_EQ(routes[0].m_status.m_jumps.size(), 1);
    ASSERT_THAT(routes[0].m_status.m_jumps, ::testing::ElementsAre(256));
    // second: indirect route
    EXPECT_EQ(routes[1].m_identifier.m_node.m_id, 3);
    EXPECT_EQ(routes[1].m_identifier.m_node.m_address, boost::asio::ip::make_address_v4("192.200.10.3"));
    EXPECT_EQ(routes[1].m_identifier.m_adapter, network_info.m_adapter);
    EXPECT_EQ(routes[1].m_identifier.m_reporter, network_info.m_sender);
    EXPECT_EQ(routes[1].m_status.m_online, true);
    ASSERT_EQ(routes[1].m_status.m_jumps.size(), 2);
    ASSERT_THAT(routes[1].m_status.m_jumps, ::testing::ElementsAre(256, 256));
}

TEST_F(route_manager_fixture, route_timeout)
{
    auto time = discnet::time_point_t::clock::now();

    discnet::network::network_info_t network_info;
    network_info.m_reception_time = time;
    network_info.m_sender = boost::asio::ip::make_address_v4("192.200.10.2");
    network_info.m_adapter = first_ip(m_adapter_1);
    network_info.m_receiver = boost::asio::ip::make_address_v4("234.5.6.7");

    discnet::network::messages::discovery_message_t message {.m_identifier = 2};
    message.m_nodes.push_back({.m_identifier = 3, .m_address = boost::asio::ip::make_address_v4("192.200.10.3"), .m_jumps {256}});
    m_route_manager->process(message, network_info);
    m_route_manager->update(time);

    auto routes = m_route_manager->find_routes_on_adapter(m_adapter_1.m_guid);
    ASSERT_EQ(routes.size(), 2);
    EXPECT_EQ(routes[0].m_status.m_online, true);
    EXPECT_EQ(routes[1].m_status.m_online, true);

    // routes should time out if there is no discovery message received within a given time frame
    m_route_manager->update(time + std::chrono::seconds(90));

    routes = m_route_manager->find_routes_on_adapter(m_adapter_1.m_guid);
    ASSERT_EQ(routes.size(), 2);
    EXPECT_EQ(routes[0].m_status.m_online, false);
    EXPECT_EQ(routes[1].m_status.m_online, false);

    network_info.m_reception_time = time + std::chrono::seconds(100);
    m_route_manager->process(message, network_info);
    m_route_manager->update(time + std::chrono::seconds(110));

    routes = m_route_manager->find_routes_on_adapter(m_adapter_1.m_guid);
    ASSERT_EQ(routes.size(), 2);
    EXPECT_EQ(routes[0].m_status.m_online, true);
    EXPECT_EQ(routes[1].m_status.m_online, true);
}

TEST_F(route_manager_fixture, persistent_node)
{
    auto time = discnet::time_point_t::clock::now();

    discnet::node_identifier_t persistent_node_id {.m_id = 1001, .m_address = boost::asio::ip::make_address_v4("192.200.10.12")};
    discnet::route_identifier_t persistent_route_id {.m_node = persistent_node_id, .m_adapter = first_ip(m_adapter_1), .m_reporter = persistent_node_id.m_address};

    { // adding persistent route
        discnet::persistent_route_t persistent_route {.m_identifier = persistent_route_id, .m_gateway = boost::asio::ip::make_address_v4("192.200.1.1"), .m_metric = 512, .m_enabled = true};
        m_route_manager->process(persistent_route, time);
    }

    discnet::node_identifier_t discovery_node_id {.m_id = 1002, .m_address = boost::asio::ip::make_address_v4("192.200.10.13")};
    discnet::route_identifier_t discovery_route_id {.m_node = discovery_node_id, .m_adapter = first_ip(m_adapter_1), .m_reporter = discovery_node_id.m_address};

    { // adding discovery route
        discnet::network::network_info_t network_info;
        network_info.m_reception_time = time;
        network_info.m_sender = discovery_node_id.m_address;
        network_info.m_adapter = first_ip(m_adapter_1);
        network_info.m_receiver = boost::asio::ip::make_address_v4("234.5.6.7");
        discnet::network::messages::discovery_message_t message {.m_identifier = discovery_node_id.m_id};
        m_route_manager->process(message, network_info);
    }
    
    m_route_manager->update(time + std::chrono::seconds(10));

    { // check route status 10 seconds after added
        auto routes = m_route_manager->find_routes_on_adapter(m_adapter_1.m_guid);
        ASSERT_EQ(routes.size(), 2);
        const auto& persistent_route = routes[0];
        EXPECT_EQ(persistent_route.m_identifier, persistent_route_id);
        EXPECT_EQ(persistent_route.m_status.m_online, true);
        EXPECT_EQ(persistent_route.m_status.m_persistent, true);
        const auto& discovery_route = routes[1];
        EXPECT_EQ(discovery_route.m_identifier, discovery_route_id);
        EXPECT_EQ(discovery_route.m_status.m_online, true);
        EXPECT_EQ(discovery_route.m_status.m_persistent, false);
    }
    
    m_route_manager->update(time + std::chrono::seconds(100));

    { // check route status 100 seconds after added
        auto routes = m_route_manager->find_routes_on_adapter(m_adapter_1.m_guid);
        ASSERT_EQ(routes.size(), 2);
        const auto& persistent_route = routes[0];
        EXPECT_EQ(persistent_route.m_identifier, persistent_route_id);
        EXPECT_EQ(persistent_route.m_status.m_online, true);
        EXPECT_EQ(persistent_route.m_status.m_persistent, true);
        const auto& discovery_route = routes[1];
        EXPECT_EQ(discovery_route.m_identifier, discovery_route_id);
        EXPECT_EQ(discovery_route.m_status.m_online, false);
        EXPECT_EQ(discovery_route.m_status.m_persistent, false);
    }
}

TEST(route_api, is_route_online)
{
    discnet::time_point_t time = discnet::time_point_t::clock::now();
    discnet::node_identifier_t node = {1010, boost::asio::ip::make_address_v4("192.200.1.3")};
    discnet::route_identifier_t identifier {.m_node = node, 
        .m_adapter = boost::asio::ip::make_address_v4("192.200.1.2"), .m_reporter = boost::asio::ip::make_address_v4("192.200.1.3")};
    discnet::route_status_t status {.m_online = true };
    discnet::route_t route {.m_identifier = identifier, .m_last_discovery = time, .m_status = status};

    
    EXPECT_EQ(discnet::is_route_online(route, time), true);
    time += std::chrono::seconds(89);
    EXPECT_EQ(discnet::is_route_online(route, time), true);
    time += std::chrono::seconds(1);
    EXPECT_EQ(discnet::is_route_online(route, time), false);
    time += std::chrono::seconds(1);
    EXPECT_EQ(discnet::is_route_online(route, time), false);

    route.m_status.m_persistent = true;
    EXPECT_EQ(discnet::is_route_online(route, time), true);
}

TEST(route_api, routes_contains)
{
    using node_identifier_t = discnet::node_identifier_t;

    auto adapter_ip = boost::asio::ip::make_address_v4("192.169.10.11");
    auto sender_1_ip = boost::asio::ip::make_address_v4("192.169.10.20");
    auto sender_2_ip = boost::asio::ip::make_address_v4("192.169.10.30");
    auto sender_3_ip = boost::asio::ip::make_address_v4("192.169.10.40");

    node_identifier_t node_1{1, boost::asio::ip::make_address_v4("192.169.10.10")};
    discnet::route_identifier_t route_1{node_1, adapter_ip, sender_1_ip};
    discnet::route_identifier_t route_2{node_1, adapter_ip, sender_2_ip};
    discnet::route_identifier_t route_3{node_1, adapter_ip, sender_3_ip};
    std::vector<discnet::route_identifier_t> routes = {route_1, route_2};

    EXPECT_FALSE(discnet::contains(routes, route_3));
    EXPECT_TRUE(discnet::contains(routes, route_2));
}

TEST(route_api, is_direct_node)
{
    using node_identifier_t = discnet::node_identifier_t;

    auto adapter_ip = boost::asio::ip::make_address_v4("192.169.10.11");
    node_identifier_t node{1, boost::asio::ip::make_address_v4("192.169.10.10")};

    discnet::route_identifier_t direct_route{node, adapter_ip, boost::asio::ip::make_address_v4("192.169.10.10")};
    EXPECT_TRUE(discnet::is_direct_node(direct_route));

    discnet::route_identifier_t indirect_route{node, adapter_ip, boost::asio::ip::make_address_v4("102.169.10.12")};
    EXPECT_FALSE(discnet::is_direct_node(indirect_route));
}
