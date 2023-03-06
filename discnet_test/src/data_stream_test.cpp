/*
 *
 */

#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/asio/buffer.hpp>
#include <discnet/network/messages/packet.hpp>
#include <discnet/network/data_stream.hpp>

using ipv4 = discnet::address_t;
using jumps_t = discnet::jumps_t;
using discnet::network::data_stream;
using namespace discnet::network::messages;

class data_stream_fixture : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
        m_discovery.m_nodes = { node_t{ 1025, ipv4::from_string("192.200.1.1"), jumps_t{512, 256} } };
    }

    void verify_discovery_message(const discovery_message_t& message)
    {
        EXPECT_EQ(message.m_identifier, 1025);
        ASSERT_EQ(message.m_nodes.size(), 1);
        EXPECT_EQ(message.m_nodes[0].m_address.to_string(), "192.200.1.1");
        ASSERT_EQ(message.m_nodes[0].m_jumps.size(), 2);
        EXPECT_EQ(message.m_nodes[0].m_jumps[0], 512);
        EXPECT_EQ(message.m_nodes[0].m_jumps[1], 256);
    }

    void verify_data_message_1(const data_message_t& message)
    {
        EXPECT_EQ(message.m_identifier, 1);
        std::string hex_string = discnet::bytes_to_hex_string(message.m_buffer);
        EXPECT_EQ(hex_string, "01 02 03");
    }

    void verify_data_message_2(const data_message_t& message)
    {
        EXPECT_EQ(message.m_identifier, 2);
        std::string hex_string = discnet::bytes_to_hex_string(message.m_buffer);
        EXPECT_EQ(hex_string, "04 05 06 07 08 09 0A");
    }
protected:
    discovery_message_t m_discovery = { .m_identifier = 1025 };
    data_message_t m_msg_1 = { .m_identifier = 1, .m_buffer = { 1,2,3 } };
    data_message_t m_msg_2 = { .m_identifier = 2, .m_buffer = { 4,5,6,7,8,9,10 } };
};

TEST_F(data_stream_fixture, no_packets)
{
    data_stream stream{4096};
    auto packets = stream.process();
    ASSERT_EQ(packets.size(), 0);
}

TEST_F(data_stream_fixture, empty_packet)
{
    data_stream stream{4096};
    message_list_t message_list = {};
    discnet::network::buffer_t buffer{1024};
    ASSERT_TRUE(packet_codec_t::encode(buffer, message_list));

    boost::asio::const_buffer cbuffer(buffer.data().data(), buffer.bytes_left_to_read());
    stream.handle_receive(cbuffer);
    auto packets = stream.process();
    ASSERT_EQ(packets.size(), 1);
    ASSERT_EQ(packets[0].m_messages.size(), 0);
}

TEST_F(data_stream_fixture, single_packet)
{
    data_stream stream{4096};
    message_list_t message_list = { m_discovery, m_msg_1 };

    discnet::network::buffer_t buffer{1024};
    ASSERT_TRUE(packet_codec_t::encode(buffer, message_list));

    boost::asio::const_buffer cbuffer(buffer.data().data(), buffer.bytes_left_to_read());
    stream.handle_receive(cbuffer);
    auto packets = stream.process();
    ASSERT_EQ(packets.size(), 1);
    ASSERT_EQ(packets[0].m_messages.size(), 2);
}

TEST_F(data_stream_fixture, multiple_packets)
{
    data_stream stream{4096};
    message_list_t message_list_1 = { m_discovery, m_msg_1 };
    message_list_t message_list_2 = { m_msg_2 };

    discnet::network::buffer_t buffer_packet_1{1024};
    ASSERT_TRUE(packet_codec_t::encode(buffer_packet_1, message_list_1));
    boost::asio::const_buffer cbuffer_1(buffer_packet_1.data().data(), buffer_packet_1.bytes_left_to_read());

    discnet::network::buffer_t buffer_packet_2{1024};
    ASSERT_TRUE(packet_codec_t::encode(buffer_packet_2, message_list_2));
    boost::asio::const_buffer cbuffer_2(buffer_packet_2.data().data(), buffer_packet_2.bytes_left_to_read());
    
    stream.handle_receive(cbuffer_1);
    stream.handle_receive(cbuffer_2);
    auto packets = stream.process();

    ASSERT_EQ(packets.size(), 2);
    EXPECT_EQ(packets[0].m_messages.size(), 2);
    EXPECT_EQ(packets[1].m_messages.size(), 1);

    const auto& decoded_messages_1 = packets[0].m_messages;
    ASSERT_EQ(decoded_messages_1.size(), 2);

    ASSERT_TRUE(std::holds_alternative<discovery_message_t>(decoded_messages_1[0]));
    auto decoded_discovery_message = std::get<discovery_message_t>(decoded_messages_1[0]);
    verify_discovery_message(decoded_discovery_message);

    ASSERT_TRUE(std::holds_alternative<data_message_t>(decoded_messages_1[1]));
    auto decoded_data_message_1 = std::get<data_message_t>(decoded_messages_1[1]);
    verify_data_message_1(decoded_data_message_1);

    const auto& decoded_messages_2 = packets[1].m_messages;
    ASSERT_EQ(decoded_messages_2.size(), 1);

    ASSERT_TRUE(std::holds_alternative<data_message_t>(decoded_messages_2[0]));
    auto decoded_data_message_2 = std::get<data_message_t>(decoded_messages_2[0]);
    verify_data_message_2(decoded_data_message_2);
}

TEST_F(data_stream_fixture, partial_read)
{
    data_stream stream{4096};
    message_list_t message_list = { m_discovery, m_msg_1 };

    discnet::network::buffer_t buffer{1024};
    ASSERT_TRUE(packet_codec_t::encode(buffer, message_list));

    size_t half_size = buffer.data().size() / 2;
    boost::asio::const_buffer first_half(buffer.data().data(), half_size);
    boost::asio::const_buffer second_half(buffer.data().data() + half_size, buffer.bytes_left_to_read() - first_half.size());

    stream.handle_receive(first_half);
    auto packets = stream.process();
    ASSERT_EQ(packets.size(), 0);

    stream.handle_receive(second_half);
    packets = stream.process();
    ASSERT_EQ(packets.size(), 1);
}

TEST_F(data_stream_fixture, partial_read_multiple_packets)
{
    data_stream stream{4096};
    message_list_t message_list_1 = { m_discovery, m_msg_1 };
    message_list_t message_list_2 = { m_msg_2 };

    discnet::network::buffer_t buffer_packet_1{1024};
    ASSERT_TRUE(packet_codec_t::encode(buffer_packet_1, message_list_1));
    boost::asio::const_buffer cbuffer_1(buffer_packet_1.data().data(), buffer_packet_1.bytes_left_to_read());

    discnet::network::buffer_t buffer_packet_2{1024};
    ASSERT_TRUE(packet_codec_t::encode(buffer_packet_2, message_list_2));
    boost::asio::const_buffer cbuffer_2_first_part(buffer_packet_2.data().data(), 10);
    boost::asio::const_buffer cbuffer_2_second_part(buffer_packet_2.data().data() + 10, buffer_packet_2.bytes_left_to_read() - 10);
    
    stream.handle_receive(cbuffer_1);
    stream.handle_receive(cbuffer_2_first_part);
    auto packets = stream.process();
    ASSERT_EQ(packets.size(), 1);
    EXPECT_EQ(packets[0].m_messages.size(), 2);

    stream.handle_receive(cbuffer_2_second_part);
    packets = stream.process();
    ASSERT_EQ(packets.size(), 1);
    EXPECT_EQ(packets[0].m_messages.size(), 1);
}