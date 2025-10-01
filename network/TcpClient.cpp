#include "TcpClient.h"
#include <vector>
#include "debug.h"
#include "str.h"
#include "./network.h"

TcpClient::TcpClient(const std::string& ip, bool http, bool https) : _resolver(_io_context), _socket(_io_context), _ip(ip)
{
    if (str::contains(_ip, ":"))
    {
        auto tmp = str::split(_ip, ":");
        _ip = tmp[0];
        _port = tmp[1];
    }
    else 
        _port = "";

    if (https)
        _port = "443";
    else if (http)
        _port = "80";
}
std::string TcpClient::send(const std::string& data, bool waitForResponse)
{
    lg2("IP", _ip);
    try
    {
        auto endpoints = _resolver.resolve(_ip, _port);
        boost::asio::connect(_socket, endpoints);
        boost::system::error_code err;
        _socket.write_some(boost::asio::buffer(data), err);
        if (!waitForResponse)
            return "";

        // get the server response
        //
        std::string _res;
        for (;;)
        {
            std::vector<unsigned char> buf(_read_buffer_size);
            size_t len = _socket.read_some(boost::asio::buffer(buf), err);
            if (err == boost::asio::error::eof)
                break; // response finished.
            else if (err)
                throw boost::system::system_error(err);

            for (int i=0; i<_read_buffer_size; i++)
                _res += buf[i];
        }

        return _res;
    }

    catch(const std::exception& e){throw;}
}

std::string TcpClient::sendAsHttp(const std::string& data,const std::string& path, bool waitForResponse)
{
   std::string http = network::client_request_as_http(path, data, network::POST, network::STRING);	
   return this->send(http, waitForResponse);
}

std::string TcpClient::sendAsHttp(const json& data,const std::string& path, bool waitForResponse)
{
   std::string http = network::client_request_as_http(path, data.dump(), network::POST, network::JSON);	
   return this->send(http, waitForResponse);
}
