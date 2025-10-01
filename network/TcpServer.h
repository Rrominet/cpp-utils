#pragma once
#include <boost/asio.hpp>
#include <boost/function.hpp>
using boost::asio::ip::tcp;

class TcpServer
{
    public : 
        enum Mode
        {
            IP4 = 1,
            IP6,
        };
    protected : 
        int _port;
        boost::asio::io_service _io_service;
        tcp::acceptor* _acceptor=nullptr;
        Mode _mode;
        std::string _request;
        std::string _response;

        long _bufSize = 1024*1024*16; // 16KB
        std::vector <boost::function<void (std::string)>> _onRequest;

        // the arg is the request and the returned value is the respond !
        std::vector <boost::function<std::string (std::string)>> _onRespond;

    public : 

        TcpServer(int port=1024, Mode mode=IP4);
        virtual ~TcpServer();

        virtual void run();
        virtual void parseRequest(std::string request){}

        int port() const {return _port;}
        void setPort(const int &port){_port = port;}

        boost::asio::io_service& ioservice(){return _io_service;}
        tcp::acceptor& acceptor()const{return *_acceptor;}
        Mode mode()const {return _mode;}

        long bufSize() const {return _bufSize;}
        void setBufSize(const long &bufSize){_bufSize = bufSize;}

        virtual std::string request() {return _request;}

        std::string response()const {return _response;}
        void setResponse(const std::string &response){_response = response;}

        void addOnRequest(boost::function<void (std::string)> f){_onRequest.push_back(f);}

        std::vector<boost::function<void(std::string)>>& onRequest(){return _onRequest;}

        void addOnRespond(boost::function<std::string (std::string)> f){_onRespond.push_back(f);}

        std::vector<boost::function<std::string(std::string)>>& onRespond(){return _onRespond;}

        virtual std::string formatRes(const std::string& res) const{return res;} 
        virtual void onRequestEnd(){}

};
