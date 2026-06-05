#include "./Plugin.h"
#include "files.2/files.h"

namespace ml
{
    PluginOut Plugin::exec(const std::string& executorPath, const json& data)
    {
        PluginOut out;
        out.path = _path;

        if (!files::exists(executorPath))
        {
            _state = State::ERROR;
            _error = "Executor file not found : " + executorPath;
            return out;
        }

        if (!files::exists(_path))
        {
            _state = State::ERROR;
            _error = "Plugin file not found : " + _path;
            return out;
        }

        std::string stdin_data;
        try
        {
            stdin_data = data.dump();
        }
        catch (const std::exception& e)
        {
            _state = State::ERROR;
            _error = "Failed to serialize data : " + std::string(e.what());
            return out;
        }

        auto pout = _execProcess(executorPath, stdin_data);
        out.processOut = pout;
        return out;
    }

    ProcessOut Plugin::_execProcess(const std::string& executorPath, const std::string& stdin)
    {
        boost::asio::io_context ioc;

        bp::async_pipe stdout_pipe(ioc);
        bp::async_pipe stderr_pipe(ioc);
        bp::opstream stdin_pipe;

        bp::child proc(
                executorPath,
                bp::args({_path}),
                bp::std_in  < stdin_pipe,
                bp::std_out > stdout_pipe,
                bp::std_err > stderr_pipe,
                ioc
                );

        _state = State::RUNNING;

        // Write stdin and close it so the child doesn't hang waiting for more
        stdin_pipe << stdin;
        stdin_pipe.flush();
        stdin_pipe.pipe().close();

        std::string stdout_result;
        std::string stderr_result;

        // Async read stdout
        boost::asio::async_read(stdout_pipe,
                boost::asio::dynamic_buffer(stdout_result),
                [](const boost::system::error_code&, std::size_t){});

        // Async read stderr
        boost::asio::async_read(stderr_pipe,
                boost::asio::dynamic_buffer(stderr_result),
                [](const boost::system::error_code&, std::size_t){});

        ioc.run();
        proc.wait();
        _state = State::DONE;

        ProcessOut out;
        out.stdout = stdout_result;
        out.stderr = stderr_result;
        out.exitCode = proc.exit_code();

        _state = State::NOT_STARTED;
        return out;
    }

    //f is called on another thread !
    void Plugin::exec_async(const std::string& executorPath, const json& data, const std::function<void(PluginOut)> &f)
    {
        auto f2 = [this, f, executorPath, data]
        {
            auto out = this->exec(executorPath, data);
            if (f)
                f(out);
        };
        std::thread(f2).detach();
    }

    bool operator==(const ml::Plugin& a, const ml::Plugin& b)
    {
        return a.path() == b.path();
    }

    bool operator!=(const ml::Plugin& a, const ml::Plugin& b)
    {
        return a.path() != b.path();
    }

    std::string Plugin::name() const
    {
        return files::name(_path);
    }
}
