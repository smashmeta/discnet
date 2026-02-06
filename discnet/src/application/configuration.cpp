/*
 *
 */

#include <iostream>
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <discnet/application/configuration.hpp>

namespace discnet::application
{
namespace options
{
    static const std::string g_help = "help";
    static const std::string g_node_id = "node_id";
    static const std::string g_multicast_address = "address";
    static const std::string g_multicast_port = "port";

    template <typename arg_type>
    arg_type get_command_line_argument(const std::string& arg_name, const boost::program_options::variables_map& variables, arg_type default_value = arg_type())
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
                // log.warning(result_info.str());
            }
        }

        return result;
    }
} // !namespace options

expected_configuration_t get_configuration(int arguments_count, const char** arguments_vector)
{
    namespace bpo = boost::program_options;

    if (arguments_count <= 1)
    {
        return std::unexpected("arguments missing. type --help for more information");
    }

    // whatlog::logger log("configuration");
    configuration_t result;
    bpo::variables_map variable_map;

    // setting up available program options
    bpo::options_description description("Allowed program options");
    description.add_options()(options::g_help.c_str(), "produces help message");
    description.add_options()(options::g_node_id.c_str(), bpo::value<uint16_t>(),
        "node identifier");
	description.add_options()(options::g_multicast_address.c_str(), bpo::value<std::string>(), 
        "multicast address");
    description.add_options()(options::g_multicast_port.c_str(), bpo::value<uint16_t>(), 
        "multicast port");

    // fetching program options from command line
    try
    {
        bpo::store(bpo::parse_command_line(arguments_count, arguments_vector, description), variable_map);
    }
    catch (const std::exception& ex)
    {
        std::string error_message = std::format("failed to parse command line arguments. Reason: {}", ex.what());
        return std::unexpected(error_message);
    }

    // store program options 
    bpo::notify(variable_map);		

    if (variable_map.count(options::g_help))
    {
        std::stringstream description_str;
        description_str << description;
        return std::unexpected(description_str.str());
    }
    
    if (variable_map.count(options::g_node_id))
    {
        result.m_node_id = options::get_command_line_argument<uint16_t>(
            options::g_node_id, variable_map, 0);

        if (result.m_node_id <= 0)
        {
            std::string error_message = "invalid node_id given";
            return std::unexpected(error_message);
        }
    }
    else
    {
        std::string error_message = "node_id argument not set"; 
        return std::unexpected(error_message);
    }

    if (variable_map.count(options::g_multicast_address))
    {
        std::string multicast_address = options::get_command_line_argument<std::string>(
            options::g_multicast_address, variable_map, "");

        boost::system::error_code error;
        result.m_multicast_address = boost::asio::ip::make_address_v4(multicast_address, error);

        if (error)
        {
            std::string error_message = "invalid multicast_address given"; 
            return std::unexpected(error_message);
        }
    }
    else
    {
        std::string error_message = "multicast_address argument not set"; 
        return std::unexpected(error_message);
    }

    if (variable_map.count(options::g_multicast_port))
    {
        int multicast_port = options::get_command_line_argument<uint16_t>(
            options::g_multicast_port, variable_map, 0);

        if (multicast_port <= 0)
        {
            std::string error_message = "invalid multicast_port given"; 
            return std::unexpected(error_message);
        }

        result.m_multicast_port = discnet::port_type_t(multicast_port);
    }
    else
    {
        std::string error_message = "multicast_port argument not set"; 
        return std::unexpected(error_message);
    }

    return result;
}
} // !namespace discnet::application