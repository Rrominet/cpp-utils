#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <string>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class TcpClient
{
    protected : 
        std::string _ip;
        boost::asio::io_context _io_context;
        tcp::resolver _resolver;
        tcp::socket _socket;
        std::string _port = "";

        unsigned int _read_buffer_size = 4096;

    public : 
        TcpClient() : _resolver(_io_context), _socket(_io_context){};
        TcpClient(const std::string& ip, bool http=false, bool https=false);
        ~TcpClient(){};

        //return the server response
        std::string send(const std::string& data, bool waitForResponse=true);
        std::string send(const json& data){return this->send(data.dump());}

        std::string sendAsHttp(const std::string& data, const std::string& path="/", bool waitForResponse=true);
        std::string sendAsHttp(const json& data, const std::string& path="/", bool waitForResponse=true);

        void listen(const std::function<void (const std::string& line)>& callback, const std::string& dataToSend="", const std::string& del="\n");
        void listenAsHttp(const std::function<void (const std::string& line)>& callback, const std::string& path="/", const std::string& del="\n");
};
