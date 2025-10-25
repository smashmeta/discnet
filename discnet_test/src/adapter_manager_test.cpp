/*
 *
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <discnet/route.hpp>
#include <discnet/adapter_manager.hpp>

namespace discnet::test
{
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
} // ! namespace discnet::test

class adapter_manager_fixture : public ::testing::Test
{
protected:
    using ipv4 = discnet::address_t;
public:
    virtual void SetUp() override
    {
        m_adapter_1.m_guid = boost::uuids::to_string(boost::uuids::random_generator()());
        m_adapter_1.m_mac_address = m_adapter_1.m_guid;
        m_adapter_1.m_index = 0;
        m_adapter_1.m_name = "test_adapter1";
        m_adapter_1.m_description = "description_adapter_1";
        m_adapter_1.m_enabled = false;
        m_adapter_1.m_gateway = boost::asio::ip::make_address_v4("0.0.0.0");
        m_adapter_1.m_address_list = { 
            {boost::asio::ip::make_address_v4("192.200.1.3"), boost::asio::ip::make_address_v4("255.255.255.0")},
            {boost::asio::ip::make_address_v4("192.169.40.130"), boost::asio::ip::make_address_v4("255.255.255.0")} 
        };

        m_adapter_2.m_guid = boost::uuids::to_string(boost::uuids::random_generator()());
        m_adapter_2.m_mac_address = m_adapter_2.m_guid;
        m_adapter_2.m_index = 1;
        m_adapter_2.m_name = "test_adapter_2";
        m_adapter_2.m_description = "description_adapter_2";
        m_adapter_2.m_gateway = boost::asio::ip::make_address_v4("0.0.0.0");
        m_adapter_2.m_enabled = true;
        m_adapter_2.m_address_list = { 
            {boost::asio::ip::make_address_v4("10.0.0.1"), boost::asio::ip::make_address_v4("255.255.255.0")}
        };
    }
protected:
    discnet::adapter_t m_adapter_1;
    discnet::adapter_t m_adapter_2;
};

TEST_F(adapter_manager_fixture, find_adapter)
{
    std::vector<discnet::adapter_t> adapters = {m_adapter_1, m_adapter_2};
    {	// making sure that the manager is destroyed (or else gtest will complain about memory leaks)
        auto fetcher = std::make_unique<discnet::test::adapter_fetcher_mock>();
        EXPECT_CALL(*fetcher.get(), get_adapters()).Times(1).WillOnce(testing::Return(adapters));
        discnet::adapter_manager manager { std::move(fetcher) };
        manager.update();

        auto adapter_valid_10 = manager.find_adapter(boost::asio::ip::make_address_v4("10.0.0.1"));
        ASSERT_TRUE(adapter_valid_10.has_value());
        EXPECT_EQ(adapter_valid_10.value().m_guid, m_adapter_2.m_guid);
        auto adapter_valid_192 = manager.find_adapter(boost::asio::ip::make_address_v4("192.200.1.3"));
        ASSERT_TRUE(adapter_valid_192.has_value());
        EXPECT_EQ(adapter_valid_192.value().m_name, m_adapter_1.m_name);
        auto invalid_adapter = manager.find_adapter(boost::asio::ip::make_address_v4("10.11.12.13"));
        EXPECT_FALSE(invalid_adapter.has_value());
    }
}

TEST_F(adapter_manager_fixture, update)
{
    using adapter_t = discnet::adapter_t;
    using adapter_manager = discnet::adapter_manager;

    // setting up data    
    adapter_t adapter_1_changed_name = m_adapter_1;
    adapter_1_changed_name.m_name = "changed_name";

    std::vector<adapter_t> adapters = {m_adapter_1};
    std::vector<adapter_t> adapters_changed = {adapter_1_changed_name};
    std::vector<adapter_t> adapters_empty = {};

    {	// making sure that the manager is destroyed (or else gtest will complain about memory leaks)
        auto fetcher = std::make_unique<discnet::test::adapter_fetcher_mock>();
        EXPECT_CALL(*fetcher.get(), get_adapters())
            .Times(3)
            .WillOnce(testing::Return(adapters))
            .WillOnce(testing::Return(adapters_changed))
            .WillOnce(testing::Return(adapters_empty));

        adapter_manager manager { std::move(fetcher) };
 
        discnet::test::callback_tester_t callbacks_tester;
        manager.e_new.connect(std::bind(&discnet::test::callback_tester_t::new_adapter, &callbacks_tester, std::placeholders::_1));
        manager.e_changed.connect(std::bind(&discnet::test::callback_tester_t::changed_adapter, &callbacks_tester, std::placeholders::_1, std::placeholders::_2));
        manager.e_removed.connect(std::bind(&discnet::test::callback_tester_t::removed_adapter, &callbacks_tester, std::placeholders::_1));
        
        EXPECT_CALL(callbacks_tester, new_adapter(testing::_)).Times(1);
        EXPECT_CALL(callbacks_tester, changed_adapter(testing::_, testing::_)).Times(1);
        EXPECT_CALL(callbacks_tester, removed_adapter(testing::_)).Times(1);

        // first call to update adds test adapter (see WillOnce(test adapter list))
        manager.update();
        // second call to update changes the adapter name (see WillOnce(test changed adapter list))
        manager.update();
        // third call to update removes test adapter (see WillOnce(empty adapter list))
        manager.update();
    }
}

