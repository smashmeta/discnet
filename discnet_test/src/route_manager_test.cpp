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
        m_adapter_1.m_address_list = { {ipv4::from_string("192.200.10.1"), ipv4::from_string("255.255.255.0")} };

        m_adapter_2.m_index = 2;
        m_adapter_2.m_enabled = true;
        m_adapter_2.m_multicast_enabled = true;
        m_adapter_2.m_guid = boost::uuids::random_generator()();
        m_adapter_2.m_name = "adapter_2";
        m_adapter_2.m_address_list = { {ipv4::from_string("192.200.20.2"), ipv4::from_string("255.255.255.0")} };

        m_adapter_3.m_index = 3;
        m_adapter_3.m_enabled = true;
        m_adapter_3.m_multicast_enabled = true;
        m_adapter_3.m_guid = boost::uuids::random_generator()();
        m_adapter_3.m_name = "adapter_3";
        m_adapter_3.m_address_list = { {ipv4::from_string("192.200.30.3"), ipv4::from_string("255.255.255.0")} };

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
    network_info.m_sender = ipv4::from_string("192.200.10.2");
    network_info.m_adapter = first_ip(m_adapter_1);
    network_info.m_receiver = ipv4::from_string("234.5.6.7");

    auto rcv_adapter = m_adapter_manager->find_adapter(network_info.m_adapter);
    ASSERT_TRUE(rcv_adapter);

    discnet::network::messages::discovery_message_t message {.m_identifier = 2};
    message.m_nodes.push_back({.m_identifier = 3, .m_address = ipv4::from_string("192.200.10.3"), .m_jumps {256}});
    m_route_manager->process(network_info, message);
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
    EXPECT_EQ(routes[0].m_status.m_jumps[0], 256);
    // second: indirect route
    EXPECT_EQ(routes[1].m_identifier.m_node.m_id, 3);
    EXPECT_EQ(routes[1].m_identifier.m_node.m_address, ipv4::from_string("192.200.10.3"));
    EXPECT_EQ(routes[1].m_identifier.m_adapter, network_info.m_adapter);
    EXPECT_EQ(routes[1].m_identifier.m_reporter, network_info.m_sender);
    EXPECT_EQ(routes[1].m_status.m_online, true);
    ASSERT_EQ(routes[1].m_status.m_jumps.size(), 2);
    EXPECT_EQ(routes[1].m_status.m_jumps[0], 256);
    EXPECT_EQ(routes[1].m_status.m_jumps[1], 256);
}

TEST_F(route_manager_fixture, route_timeout)
{
    auto time = discnet::time_point_t::clock::now();

    discnet::network::network_info_t network_info;
    network_info.m_reception_time = time;
    network_info.m_sender = ipv4::from_string("192.200.10.2");
    network_info.m_adapter = first_ip(m_adapter_1);
    network_info.m_receiver = ipv4::from_string("234.5.6.7");

    auto rcv_adapter = m_adapter_manager->find_adapter(network_info.m_adapter);
    ASSERT_TRUE(rcv_adapter);

    discnet::network::messages::discovery_message_t message {.m_identifier = 2};
    message.m_nodes.push_back({.m_identifier = 3, .m_address = ipv4::from_string("192.200.10.3"), .m_jumps {256}});
    m_route_manager->process(network_info, message);
    m_route_manager->update(time);

    auto routes = m_route_manager->find_routes_on_adapter(rcv_adapter.value().m_guid);
    ASSERT_EQ(routes.size(), 2);
    EXPECT_EQ(routes[0].m_status.m_online, true);
    EXPECT_EQ(routes[1].m_status.m_online, true);

    // routes should time out if there is no discovery message received within a given time frame
    m_route_manager->update(time + std::chrono::seconds(90));

    routes = m_route_manager->find_routes_on_adapter(rcv_adapter.value().m_guid);
    ASSERT_EQ(routes.size(), 2);
    EXPECT_EQ(routes[0].m_status.m_online, false);
    EXPECT_EQ(routes[1].m_status.m_online, false);
}