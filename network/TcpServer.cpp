#include "TcpServer.h"
#include <exception>
#include <mutex>
#include "debug.h"

TcpServer::TcpServer(int port, Mode mode, bool async) : 
    _port(port), _mode{mode}, _async{async}
{
    if (_mode == IP4)
        _acceptor = std::make_unique<tcp::acceptor>(_io_service, tcp::endpoint(tcp::v4(), port));
    else if (_mode == IP6)
        _acceptor = std::make_unique<tcp::acceptor>(_io_service, tcp::endpoint(tcp::v6(), port));
}

TcpServer::~TcpServer(){}

void TcpServer::run()
{
    lg("TcpServer::run() - Port " << _port);
    try
    {
        for (;;)
        {
            lg("Accepting the socket...");
            std::shared_ptr<tcp::socket> s = std::make_shared<tcp::socket>(_io_service);
            _acceptor->accept(*s);
            lg("Socket accepted.");
            if (!_async)
                this->handleSocket(s);
            else 
                _pool.run([this, s]{this->handleSocket(s);});
        }
    }

    catch(const std::exception& e)
    {
        lg("Error in the server loop : " << e.what());
        _execOnError(e.what());
    }
}

void TcpServer::setSocketTimeouts(tcp::socket& socket, int read_seconds, int write_seconds)
{
	timeval read_timeout;
    read_timeout.tv_sec = read_seconds;
    read_timeout.tv_usec = 0;

    ::setsockopt(
        socket.native_handle(),
        SOL_SOCKET,
        SO_RCVTIMEO,
        &read_timeout,
        sizeof(read_timeout)
    );

	timeval write_timeout;
    write_timeout.tv_sec = write_seconds;
    write_timeout.tv_usec = 0;

    ::setsockopt(
        socket.native_handle(),
        SOL_SOCKET,
        SO_SNDTIMEO,
        &write_timeout,
        sizeof(write_timeout)
    );
}

void TcpServer::handleSocket(std::shared_ptr<tcp::socket> s)
{
    lg("Settings the timeouts");
    TcpServer::setSocketTimeouts(*s, _readSocketTimeout, _writeSocketTimeout);
    std::string request;
    boost::system::error_code error;
    while(!error)
    {
        std::vector<char> buf(_bufSize);
        size_t len = s->read_some(boost::asio::buffer(buf), error);
        if (error == boost::asio::error::eof)
        {
            lg("EOF Error");
            break; // Connection closed cleanly by peer.
        }
        else if (error)
        {
            lg("Other error... (system_error) : " << error.message());
            _execOnError(error.message());
            break;
        }

        for (int i=0; i<len; i++)
            request.push_back(buf[i]);
    }

    lg2("Request received", request);
    std::vector <std::function<std::string (const std::string&)>> onrequest;
    {
        std::lock_guard lk(_onRequest);
        onrequest = _onRequest.data();
    }
    for (auto &f : onrequest)
        this->write(s, f(request));
}

void TcpServer::write(std::shared_ptr<tcp::socket> s,const std::string& res)
{
    boost::system::error_code error;
    this->write(s, res, error);
}

void TcpServer::write(std::shared_ptr<tcp::socket> s,const std::string& res,boost::system::error_code& error)
{
    for (size_t j=0; j<res.size(); j+=_bufSize)
    {
        std::string _res = res.substr(j, _bufSize);
        boost::asio::write(*s, boost::asio::buffer(_res), error);
        if (error)
        {
            _execOnError(error.message());
            return;
        }
    }
}

void TcpServer::_execOnError(const std::string& error)
{
    std::vector <std::function<void (const std::string&)>> onerror;
    {
        std::lock_guard lk(_onError);
        onerror = _onError.data();
    }
    for (auto &f : onerror)
        f(error);
}

void TcpServer::addOnRequest(std::function<std::string (const std::string&)> f)
{
    std::lock_guard lk(_onRequest);
    _onRequest.data().push_back(f);
}

void TcpServer::addOnError(std::function<void (const std::string&)> f)
{
    std::lock_guard lk(_onError);
    _onError.data().push_back(f);
}

