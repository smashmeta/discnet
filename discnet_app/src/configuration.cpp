/*
 *
 */

#include <iostream>
#include <strstream>
#include <fmt/format.h>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <whatlog/logger.hpp>
#include <discnet_app/configuration.hpp>

namespace discnet::app
{
namespace options
{
    static const std::string g_node_id = "node_id";
    static const std::string g_multicast_address = "address";
    static const std::string g_multicast_port = "port";
    static const std::string g_adapter = "adapter";

    template <typename arg_type>
    arg_type get_command_line_argument(whatlog::logger& log, const std::string& arg_name, const boost::program_options::variables_map& variables, arg_type default_value = arg_type())
    {
        arg_type result = default_value;
        std::stringstream result_info;

        if (variables.count(arg_name))
        {
            try
            {
                result = variables[arg_name].as<arg_type>();
            }
            catch (const std::exception& ex)
            {
                // report warning to client about mismatching argument type
                result_info << "failed to convert console argument option '" << arg_name << "' to type " <<
                    boost::typeindex::type_id<arg_type>().pretty_name() << ", exception: " << ex.what() << std::endl;
                log.warning(result_info.str());
            }
        }

        return result;
    }
} // !namespace options

bool initialize_console_logger()
{
    try
    {
        whatlog::logger::initialize_console_logger();
    }
    catch (const std::exception& ex)
    {
        std::cout << fmt::format("failed to initialize console logger. exception: {}", ex.what()) << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "failed to initialize console logger. unknown exception! " << std::endl;
        return false;
    }

    return true;
}

std::expected<configuration_t, std::string> get_configuration(int arguments_count, const char** arguments_vector)
{
    using ipv4 = boost::asio::ip::address_v4;
    namespace bpo = boost::program_options;

    whatlog::logger log("configuration");
    configuration_t result;
    bpo::variables_map variable_map;

    // setting up available program options
    bpo::options_description description("Allowed program options");
    description.add_options()(discnet::app::options::g_node_id.c_str(), bpo::value<uint16_t>(),
        "node identifier");
	description.add_options()(discnet::app::options::g_multicast_address.c_str(), bpo::value<std::string>(), 
        "multicast address");
    description.add_options()(discnet::app::options::g_multicast_port.c_str(), bpo::value<uint16_t>(), 
        "multicast port");

    // fetching program options from command line
    try
    {
        bpo::store(bpo::parse_command_line(arguments_count, arguments_vector, description), variable_map);
        log.info("command line arguments parsed.");
    }
    catch (const std::exception& ex)
    {
        std::string error_message = fmt::format("failed to parse command line arguments. Reason: {}", ex.what());
        log.error(error_message);
        return std::unexpected(error_message);
    }

    // store program options 
    bpo::notify(variable_map);		
    
    if (variable_map.count(discnet::app::options::g_node_id))
    {
        result.m_node_id = discnet::app::options::get_command_line_argument<uint16_t>(
            log, discnet::app::options::g_node_id, variable_map, 0);

        if (result.m_node_id <= 0)
        {
            std::string error_message = "invalid node_id given.";
            log.error(error_message);
            return std::unexpected(error_message);
        }
    }
    else
    {
        std::string error_message = "node_id argument not set."; 
        log.error(error_message);
        return std::unexpected(error_message);
    }

    if (variable_map.count(discnet::app::options::g_multicast_address))
    {
        std::string multicast_address = discnet::app::options::get_command_line_argument<std::string>(
            log, discnet::app::options::g_multicast_address, variable_map, "");

        boost::system::error_code error;
        result.m_multicast_address = ipv4::from_string(multicast_address, error);

        if (error)
        {
            std::string error_message = "invalid multicast_address given."; 
            log.error(error_message);
            return std::unexpected(error_message);
        }
    }
    else
    {
        std::string error_message = "multicast_address argument not set."; 
        log.error(error_message);
        return std::unexpected(error_message);
    }

    if (variable_map.count(discnet::app::options::g_multicast_port))
    {
        int multicast_port = discnet::app::options::get_command_line_argument<uint16_t>(
            log, discnet::app::options::g_multicast_port, variable_map, 0);

        if (multicast_port <= 0)
        {
            std::string error_message = "invalid multicast_port given."; 
            log.error(error_message);
            return std::unexpected(error_message);
        }

        result.m_multicast_port = discnet::port_type_t(multicast_port);
    }
    else
    {
        std::string error_message = "multicast_port argument not set."; 
        log.error(error_message);
        return std::unexpected(error_message);
    }

    return result;
}
} // !namespace discnet::app