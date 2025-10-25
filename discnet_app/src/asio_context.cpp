/*
 *
 */

#include <chrono>
#include <spdlog/spdlog.h>
#include <discnet_app/asio_context.hpp>
#include <thread>


namespace discnet::main
{
    asio_context_t::asio_context_t()
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
            spdlog::info("spawning asio worker thread - named: {}.", thread_names[i]);
            m_thread_group.push_back(std::jthread(std::bind(&work_handler, thread_names[i], m_io_context)));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void asio_context_t::work_handler(const std::string& thread_name, discnet::shared_io_context io_context)
    {
        // whatlog::rename_thread(GetCurrentThread(), thread_name);
        // whatlog::logger log("work_handler");
        spdlog::info("starting thread {}.", thread_name);

        for (;;)
        {
            try
            {
                io_context->run();
                break;
            }
            catch (std::exception& ex)
            {
                spdlog::warn("worker thread encountered an error. exception: {}.", std::string(ex.what()));
            }
            catch (...)
            {
                spdlog::warn("worker thread encountered an unknown exception.");
            }
        }
    }
} // ! namespace discnet::main