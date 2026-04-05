#pragma once
#include "../thread.h"
#include <boost/asio.hpp>
#include <functional>
using boost::asio::ip::tcp;

class TcpServer
{
    public : 
        enum Mode
        {
            IP4 = 1,
            IP6,
        };

        TcpServer(int port=1024, Mode mode=IP4, bool async=false);
        virtual ~TcpServer();

        virtual void run();

        boost::asio::io_service& ioservice(){return _io_service;}
        tcp::acceptor& acceptor()const{return *_acceptor;}

        void addOnRequest(std::function<std::string (const std::string&)> f);
        void addOnError(std::function<void (const std::string&)> f);

        virtual void handleSocket(std::shared_ptr<tcp::socket> s);
        void write(std::shared_ptr<tcp::socket> s, const std::string& res);
        void write(std::shared_ptr<tcp::socket> s, const std::string& res, boost::system::error_code& error);

    protected : 
        int _port; //bp cgs
        boost::asio::io_service _io_service;
        std::unique_ptr<tcp::acceptor> _acceptor=nullptr;
        Mode _mode; //bp cg
                    
        bool _async = false; //bp cg
        th::ThreadPool _pool;

        long _bufSize = 1024*1024*16; //bp cgs

        //the arg is the request received and the return is the response to send back.
        th::Safe<std::vector <std::function<std::string (const std::string&)>>> _onRequest;
        th::Safe<std::vector <std::function<void (const std::string&)>>> _onError;

        void _execOnError(const std::string& error);

    public : 
#include "./TcpServer_gen.h"
};
