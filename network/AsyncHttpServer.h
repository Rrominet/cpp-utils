#pragma once
#include "./HttpServer.h"

//this class is here for compatibility
//just use HttpServer(port, mode, true) for an async server.
class AsyncHttpServer : public HttpServer
{
    public:
        AsyncHttpServer(int port=8080, Mode mode=IP4) : HttpServer(port, mode, true){}
        virtual ~AsyncHttpServer(){}
};
