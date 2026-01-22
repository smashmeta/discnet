/*
 *
 */

#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <boost/uuid/random_generator.hpp>
#include <boost/thread.hpp>
#include <spdlog/spdlog.h>
#include <discnet/route.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/route_manager.hpp>
#include <discnet/node.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/messages/packet.hpp>
#include <discnet/network/messages/discovery_message.hpp>
#include <discnet/network/multicast_client.hpp>
#include <discnet/application/configuration.hpp>


namespace discnet::test
{
    template<typename ... Ts>
    std::vector<std::byte> make_bytes(Ts&&... args) noexcept
    {
        return { std::byte(std::forward<Ts>(args))... };
    }
}

TEST(main, shift_buffer_debugging_remove_later)
{
    auto buffer = discnet::test::make_bytes(1, 2, 3, 4, 5, 6);
    std::shift_left(buffer.begin(), buffer.end(), 3);
    buffer.resize(3);
    std::string hex_string = discnet::bytes_to_hex_string(buffer);
    EXPECT_EQ(hex_string, "04 05 06");
}

TEST(main, sha256_file)
{
    std::string hashed_file = discnet::sha256_file("sha.file");
    EXPECT_EQ(hashed_file, "f33ae3bc9a22cd7564990a794789954409977013966fb1a8f43c35776b833a95");
}

TEST(arguments_parsing, normal_usage)
{
    const char* arguments[] = {"c:\\my_program_path.exe", "--node_id", "1234", "--address", "234.5.6.7", "--port", "1337"};
    auto expected_configuration = discnet::application::get_configuration(7, arguments);
    ASSERT_TRUE(expected_configuration.has_value());

    discnet::application::configuration_t configuration = expected_configuration.value();
    EXPECT_EQ(configuration.m_node_id, 1234);
    EXPECT_EQ(configuration.m_multicast_address, boost::asio::ip::make_address_v4("234.5.6.7"));
    EXPECT_EQ(configuration.m_multicast_port, 1337);
}

TEST(arguments_parsing, missing_arguments)
{
    const char* arguments[] = {"c:\\my_program_path.exe"};
    auto expected_configuration = discnet::application::get_configuration(1, arguments);
    ASSERT_FALSE(expected_configuration.has_value());
    EXPECT_EQ(expected_configuration.error(), "arguments missing. type --help for more information");
}

TEST(arguments_parsing, help)
{
    const char* arguments[] = {"c:\\my_program_path.exe", "--help"};
    auto expected_configuration = discnet::application::get_configuration(2, arguments);
    ASSERT_FALSE(expected_configuration.has_value());
    ASSERT_THAT(expected_configuration.error(), ::testing::StartsWith("Allowed program options:"));
}

TEST(arguments_parsing, invalid_parameters_node_id)
{
    const char* arguments[] = {"c:\\my_program_path.exe", "--node_id", "abcd", "--address", "234.5.6.7", "--port", "1337"};
    auto expected_configuration = discnet::application::get_configuration(7, arguments);
    ASSERT_FALSE(expected_configuration.has_value());
}

TEST(arguments_parsing, invalid_parameters_address)
{
    const char* arguments[] = {"c:\\my_program_path.exe", "--node_id", "1234", "--address", "334.33.6.7", "--port", "1337"};
    auto expected_configuration = discnet::application::get_configuration(7, arguments);
    ASSERT_FALSE(expected_configuration.has_value());
    EXPECT_EQ(expected_configuration.error(), "invalid multicast_address given");
}

TEST(main, bytes_to_hex_string)
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

    {   // single byte 
        std::vector<std::byte> buffer = discnet::test::make_bytes(16);
        std::string hex_string = discnet::bytes_to_hex_string(buffer);
        EXPECT_EQ(hex_string, "10");
    }

    {   // single byte (padded)
        std::vector<std::byte> buffer = discnet::test::make_bytes(8);
        std::string hex_string = discnet::bytes_to_hex_string(buffer);
        EXPECT_EQ(hex_string, "08");
    }
}

TEST(main, buffer_t__packet)
{
    using jumps_t = discnet::jumps_t;
    using discnet::node_identifier_t;
    using discnet::network::buffer_t;
    using namespace discnet::network::messages;

    discovery_message_t discovery_message {.m_identifier = 1024};
    discovery_message.m_nodes = {node_t{1025, boost::asio::ip::make_address_v4("192.200.1.1"), jumps_t{512, 256}}};

    data_message_t data_message {.m_identifier = 1};
    data_message.m_buffer = {1, 2, 3, 4, 5};

    buffer_t buffer(1024);
    message_list_t messages = {discovery_message, data_message};
    ASSERT_TRUE(packet_codec_t::encode(buffer, messages));

    std::string output = discnet::bytes_to_hex_string(buffer.data());

    // packet: size + message count :	{ 00 00 00 2F, 00 02 } = { 32, 2 }
    // discovery message:
    // header: size + type :			{ 00 00 00 16, 00 01 } = { 22, 1 }
    // discovery id + element count :	{ 04 00, 00 01 } = { 1024, 1 }
    // element 1 (identifier) :			{ 04 01, C0 C8 01 01 } = { 1025, 192.200.1.1 }
    // element 1 (status) :				{ 00 02, 02 00, 01 00 } = { 2, 512, 256 }
    // data message:
    // header: size + type :			{ 00 00 00 0F, 00 02 } = { 15, 2 }
    // data id + size :			    	{ 00 01 00 05 } = { 1, 5 }
    // buffer :							{ 01 02 03 04 05 }= { 1, 2, 3, 4, 5 }
    // checksum :
    // checksum value :					{ F0 75 B0 16 } = 4-bytes
    EXPECT_EQ(output, 
        "00 00 00 2F 00 02 "
        "00 00 00 16 00 01 "
        "04 00 00 01 "
        "04 01 C0 C8 01 01 "
        "00 02 02 00 01 00 "
        "00 00 00 0F 00 02 "
        "00 01 00 05 "
        "01 02 03 04 05 "
        "F0 75 B0 16");

    expected_packet_t packet = packet_codec_t::decode(buffer);
    ASSERT_TRUE(packet.has_value()) << packet.error();

    const auto& decoded_messages = packet.value().m_messages;
    ASSERT_EQ(decoded_messages.size(), 2);
    
    ASSERT_TRUE(std::holds_alternative<discovery_message_t>(decoded_messages[0]));
    auto decoded_discovery_message = std::get<discovery_message_t>(decoded_messages[0]);
    EXPECT_EQ(decoded_discovery_message, discovery_message);

    ASSERT_TRUE(std::holds_alternative<data_message_t>(decoded_messages[1]));
    auto decoded_data_message = std::get<data_message_t>(decoded_messages[1]);
    EXPECT_EQ(decoded_data_message, data_message);
}

int main(int arguments_count, char** arguments_vector) 
{
    spdlog::set_level(spdlog::level::off);
    testing::InitGoogleTest(&arguments_count, arguments_vector);
    return RUN_ALL_TESTS();
}