#pragma once
#include "../Events.h"
#include "../thread.h"
#include "./HttpServer.h"
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

class SSEEventLoop
{
    public : 
        SSEEventLoop(HttpServer* server) : _server(server) {_init();}
        ~SSEEventLoop();
        void run();
        void stop();

        void addSubscriber(std::shared_ptr<tcp::socket> s);
        void removeSubscriber(std::shared_ptr<tcp::socket> s);

        void emit(const json& data);

    private : 
        ml::Events _events; //bp cg
        th::Safe<ml::Vec<std::shared_ptr<tcp::socket>>> _subscribers;

        std::atomic<bool> _stop = false;
        HttpServer* _server = nullptr;
        void _init();

    public : 
#include "SSEEventLoop_gen.h"
};
