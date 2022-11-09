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
    typedef boost::system::error_code error_code_t;
    typedef boost::asio::ip::address_v4 address_v4_t;
    typedef boost::asio::ip::port_type port_type_t;
    typedef boost::asio::ip::udp::socket socket_t;
    typedef std::chrono::system_clock::time_point time_point_t;
    typedef boost::uuids::uuid uuid_t;
    typedef std::pair<address_v4_t, address_v4_t> address_mask_v4_t; 
    typedef std::shared_ptr<boost::asio::io_service> shared_io_service;
    typedef std::shared_ptr<boost::asio::ip::udp::socket> shared_udp_socket;

    struct init_required_t 
    {
        template <class T>
        operator T() const { static_assert(typeof(T) == 0, "struct memeber not initialized"); }
    } static const init_required;

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