/*
 *
 */

#pragma once

#include <vector>
#include <memory>
#include <discnet/typedefs.hpp>


namespace discnet::main
{
    struct asio_context_t
    {
        asio_context_t();

        static void work_handler(const std::string& thread_name, discnet::shared_io_context io_context);
        
        std::shared_ptr<boost::asio::io_context> m_io_context;
        std::shared_ptr<boost::asio::io_context::work> m_worker;
        boost::thread_group m_thread_group;
    };
} // ! namespace discnet::main