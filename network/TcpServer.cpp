#include "TcpServer.h"
#include <exception>
#include "debug.h"

TcpServer::TcpServer(int port, Mode mode) : 
    _port(port), _mode{mode}
{
    try
    {
        if (_mode == IP4)
            _acceptor = new tcp::acceptor(_io_service, tcp::endpoint(tcp::v4(), port));
        else if (_mode == IP6)
            _acceptor = new tcp::acceptor(_io_service, tcp::endpoint(tcp::v6(), port));
    }
    catch(const std::exception& e){throw;}
}

TcpServer::~TcpServer()
{
    delete _acceptor;
}

void TcpServer::run()
{
    try
    {
        for (;;)
        {
            tcp::socket s(_io_service);
            _acceptor->accept(s);

            boost::system::error_code error;
            std::vector<char> buf(_bufSize);
            size_t len = s.read_some(boost::asio::buffer(buf), error);
            if (error == boost::asio::error::eof)
            {
                lg("EOF Error");
                break; // Connection closed cleanly by peer.
            }
            else if (error)
            {
                lg("Other error... (system_error)");
                throw boost::system::system_error(error); // Some other error.
            }

            for (int i=0; i<len; i++)
                _request.push_back(buf[i]);

            lg2("Request sended", _request);
            this->parseRequest(this->request());
            for (auto &f : _onRequest)
                f(_request);

            for (auto &f : _onRespond)
            {
                auto res = f(this->request());
                res = this->formatRes(res);
                lg2("Response", res);
                boost::asio::write(s, boost::asio::buffer(res), error);

                lg("Socket written !");
            }

            this->onRequestEnd();
            _request = "";
        }
    }

    catch(const std::exception& e){lg("Error in the server loop"); throw;}
            
}
