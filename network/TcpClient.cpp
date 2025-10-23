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

void TcpClient::listen(const std::function<void (const std::string& line)>& callback,const std::string& dataToSend, const std::string& del)
{
    try
    {
        auto endpoints = _resolver.resolve(_ip, _port);
        boost::asio::connect(_socket, endpoints);
        boost::system::error_code err;

        if (!dataToSend.empty())
            _socket.write_some(boost::asio::buffer(dataToSend), err);
        else
            _socket.write_some(boost::asio::buffer(del), err);

        boost::asio::streambuf buffer;
        for (;;)
        {
            // read *whatever's available* (doesn't force a delimiter)
            std::size_t n = boost::asio::read(_socket, buffer.prepare(_read_buffer_size),
                    boost::asio::transfer_at_least(1), err);
            if (err == boost::asio::error::eof) {
                std::cout << "Server closed connection\n";
                break;
            } else if (err) {
                std::cerr << "Error: " << err.message() << "\n";
                break;
            }

            buffer.commit(n);

            // Now parse the buffer manually, like curl does
            std::istream is(&buffer);
            std::string line;
            while (std::getline(is, line)) {
                callback(line);
            }

        }
    }

    catch(const std::exception& e){throw;}
}

void TcpClient::listenAsHttp(const std::function<void (const std::string& line)>& callback, const std::string& path, const std::string& del)
{
    std::string http = network::client_request_as_http(path, "", network::GET, network::STRING);	
    this->listen(callback, http, del);
}
