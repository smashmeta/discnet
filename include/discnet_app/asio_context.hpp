/*
 *
 */

#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <discnet/typedefs.hpp>


namespace discnet::main
{
    struct asio_context_t
    {
        asio_context_t();

        static void work_handler(const std::string& thread_name, discnet::shared_io_context io_context);
        
        std::shared_ptr<boost::asio::io_context> m_io_context;
        std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> m_worker;
        std::vector<std::jthread> m_thread_group;
    };
} // ! namespace discnet::main