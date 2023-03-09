/*
 *
 */

#include <whatlog/logger.hpp>
#include <discnet_app/asio_context.hpp>


namespace discnet::main
{
    asio_context_t::asio_context_t()
    {
        whatlog::logger log("asio_context::ctor");

        std::vector<std::string> thread_names = {"mercury", "venus"};
        const size_t worker_threads_count = thread_names.size();
        m_io_context = std::make_shared<boost::asio::io_context>((int)worker_threads_count);
        // worker keeps the asio context running. otherwise io context would stop
        // when there is no more work to do.
        m_worker = std::make_shared<boost::asio::io_context::work>(*m_io_context);
        
        for (size_t i = 0; i < worker_threads_count; ++i)
        {
            log.info("spawning asio worker thread - named: {}.", thread_names[i]);
            m_thread_group.create_thread(boost::bind(&work_handler, thread_names[i], m_io_context));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void asio_context_t::work_handler(const std::string& thread_name, discnet::shared_io_context io_context)
    {
        whatlog::rename_thread(GetCurrentThread(), thread_name);
        whatlog::logger log("work_handler");
        log.info("starting thread {}.", thread_name);

        for (;;)
        {
            try
            {
                boost::system::error_code error_code;
                io_context->run(error_code);
                if (error_code)
                {
                    log.error("worker thread encountered an error. message: ", error_code.message());
                }

                break;
            }
            catch (std::exception& ex)
            {
                log.warning("worker thread encountered an error. exception: {}.", std::string(ex.what()));
            }
            catch (...)
            {
                log.warning("worker thread encountered an unknown exception.");
            }
        }
    }
} // ! namespace discnet::main