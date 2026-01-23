/*
 *
 */

#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <discnet/typedefs.hpp>
#include <discnet/application/configuration.hpp>


namespace discnet::main
{
    struct asio_context_t
    {
        asio_context_t(const discnet::application::shared_loggers& loggers);

        static void work_handler(discnet::application::shared_loggers loggers, const std::string& thread_name, discnet::shared_io_context io_context);
        
        discnet::application::shared_loggers m_loggers;
        std::shared_ptr<boost::asio::io_context> m_io_context;
        std::shared_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> m_worker;
        std::vector<std::jthread> m_thread_group;
    };
} // ! namespace discnet::main