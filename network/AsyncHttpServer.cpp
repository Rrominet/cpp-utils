#include "./AsyncHttpServer.h"
#include <mutex>
using namespace std::chrono_literals;

void AsyncHttpServer::execOnRequest(const std::string& request)
{
    std::vector <boost::function<void (std::string)>> to_exec;
    {
        std::lock_guard<std::mutex> lock(_onRequestMtx);
        to_exec = _onRequest;
    }

    db::write("AsyncHttpServer::execOnRequest");
    if (to_exec.empty())
        return;

    std::string req = request;
    db::write("received request : " + req);
    auto f = [to_exec, req]
    {
        for (auto& _f : to_exec)
        {
            db::write("executing callback on previous request.");
            _f(req);
        }

        db::write("All callbacks executed.");
    };
}

void AsyncHttpServer::execOnRespond(tcp::socket& socket, std::unordered_map<std::string, std::string>& httpdata, boost::system::error_code& error)
{
    std::vector <boost::function<std::string (std::string)>> to_exec;
    ml::Vec<std::function<std::string(std::unordered_map<std::string, std::string>)>> to_http_exec;
    {
        std::lock_guard<std::mutex> lk(_onRespondMtx);
        to_exec = _onRespond;
        to_http_exec = this->httpResponses;
    }

    for (auto &f : to_exec)
    {
        auto res = f(this->request(httpdata));
        res = this->formatRes(res);
        res += "\n";
        this->write(socket, res, error);
    }

    for (auto &f : to_http_exec)
    {
        auto res = f(httpdata);
        res = this->formatRes(res);
        res += "\n";
        this->write(socket, res, error);
    }
}

void AsyncHttpServer::run()
{
    _state.setRunning(true);
    try
    {
        while(_state.isRunning())
        {
            auto socket = std::make_unique<tcp::socket>(_io_service);
            try
            {
                _acceptor->accept(*socket); // this actually wait for a new connection
                _state.incrementConnections();
                auto f = [this, socket = std::move(socket)]() mutable
                {
                    try
                    {
                        this->onRequestLoop(*socket, _bufSize);
                    }
                    catch(const std::exception& e)
                    {
                        db_write("Error from onRequestLoop : " + std::string(e.what()));
                    }
                    _state.decrementConnections();
                };
                std::thread(std::move(f)).detach();
            }
            catch(const boost::system::system_error& e)
            {
                if (e.code() == boost::asio::error::operation_aborted)
                {
                    if (_state.isRunning())
                    {
                        if (!this->attemptRecovery())
                        {
                            db_write("Server failed to recover from crash.");
                            throw;
                        }
                    }
                }
            }
        }
    }

    catch(const std::exception& e)
    {
        db_write("Fatal error in the server loop : " + std::string(e.what()));
        if (_onQuit)
            _onQuit();
        throw;
    }
}

bool AsyncHttpServer::attemptRecovery()
{
    if (_onQuit)
        _onQuit();
    int retry_count = 0;
    while (retry_count++ < _max_retry) 
    {
        try {
            _io_service.reset();
            _acceptor->close();
            _acceptor->open(tcp::v4());
            _acceptor->bind(tcp::endpoint(tcp::v4(), _port));
            _acceptor->listen();
            return true;
        } catch (const std::exception& e) {
            db_write("Recovery attempt " + std::to_string(retry_count) + " failed: " + e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return false;
}
