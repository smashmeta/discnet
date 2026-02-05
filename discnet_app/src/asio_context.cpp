/*
 *
 */

#include <chrono>
#include <spdlog/spdlog.h>
#include "asio_context.hpp"
#include <thread>


namespace discnet::main
{
    asio_context_t::asio_context_t(const discnet::application::shared_loggers& loggers)
        : m_loggers(loggers)
    {
        // whatlog::logger log("asio_context::ctor");

        std::vector<std::string> thread_names = {"mercury", "venus"};
        const size_t worker_threads_count = thread_names.size();
        m_io_context = std::make_shared<boost::asio::io_context>((int)worker_threads_count);
        // worker keeps the asio context running. otherwise io context would stop
        // when there is no more work to do.
        m_worker = std::make_shared<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(m_io_context->get_executor());
        
        for (size_t i = 0; i < worker_threads_count; ++i)
        {
            m_loggers->m_logger->info("spawning asio worker thread - named: {}.", thread_names[i]);
            m_thread_group.push_back(std::jthread(std::bind(&work_handler, m_loggers, thread_names[i], m_io_context)));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void asio_context_t::work_handler(discnet::application::shared_loggers loggers, const std::string& thread_name, discnet::shared_io_context io_context)
    {
        loggers->m_logger->info("starting thread {}.", thread_name);

        for (;;)
        {
            try
            {
                io_context->run();
                break;
            }
            catch (std::exception& ex)
            {
                loggers->m_logger->warn("worker thread encountered an error. exception: {}.", std::string(ex.what()));
            }
            catch (...)
            {
                loggers->m_logger->warn("worker thread encountered an unknown exception.");
            }
        }
    }
} // ! namespace discnet::main