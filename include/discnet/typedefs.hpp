/*
 *
 */

#pragma once

#include <memory>
#include <boost/endian.hpp>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <discnet/discnet.hpp>

namespace discnet
{
    typedef uint8_t byte_t;
    typedef unsigned long mtu_type_t;
    typedef boost::system::error_code error_code_t;
    typedef boost::asio::ip::address_v4 address_t;
    typedef boost::asio::ip::port_type port_type_t;
    typedef boost::asio::ip::udp::endpoint endpoint_t;
    typedef boost::asio::ip::udp::socket socket_t;
    typedef std::chrono::system_clock::time_point time_point_t;
    typedef std::chrono::system_clock::duration duration_t;
    using  sys_clock_t = std::chrono::system_clock;
    typedef std::chrono::duration<float, std::milli> duration_ms_t;
    typedef std::pair<address_t, address_t> address_mask_t; 
    typedef std::shared_ptr<boost::asio::io_context> shared_io_context;
    typedef std::shared_ptr<boost::asio::ip::udp::socket> shared_udp_socket;
    using metric_t = uint16_t;
    using jumps_t = std::vector<metric_t>;

    DISCNET_EXPORT std::string to_string(const jumps_t& jumps);

#ifdef _WIN32
	#pragma warning( push )
	#pragma warning( disable : 4996 )
#elif defined(__GNUC__) && !defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wlanguage-extension-token"
#else
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif
    struct init_required_t 
    {
        template <class T>
        operator T() const { static_assert(false, "struct memeber not initialized"); }
    } static const init_required;
#ifdef _WIN32
	#pragma warning( pop )
#elif defined(__GNUC__) && !defined(__clang__)
	#pragma GCC diagnostic pop
#else
	#pragma clang diagnostic pop
#endif

    namespace network
    {
        template <typename ... args_t>
        auto native_to_network(args_t&&... args) -> decltype(boost::endian::native_to_big(std::forward<args_t>(args)...))
        {
            return boost::endian::native_to_big(std::forward<args_t>(args)...);
        }

        template <typename ... args_t>
        auto network_to_native(args_t&&... args) -> decltype(boost::endian::big_to_native(std::forward<args_t>(args)...))
        {
            return boost::endian::big_to_native(std::forward<args_t>(args)...);
        }
    } // ! namespace network
} // ! namespace discnet