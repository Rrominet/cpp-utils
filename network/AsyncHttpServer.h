#pragma once
#include "./HttpServer.h"
#include "../thread.h"
#include "./network.h"

// basic documentation is in HttpServer.h

class ServerState 
{
private:
    std::atomic<bool> running{false};
    std::atomic<int> active_connections{0};
    std::atomic<bool> graceful_shutdown{false};
    
public:
    void incrementConnections() { active_connections++; }
    void decrementConnections() { active_connections--; }
    int getActiveConnections() const { return active_connections; }
    bool isRunning() const { return running; }
    void setRunning(bool state) { running = state; }
    void initiateGracefulShutdown() { graceful_shutdown = true; }
};

class AsyncHttpServer : public HttpServer
{
    public:
        AsyncHttpServer(int port=8080, Mode mode=IP4) : HttpServer(port, mode){}
        virtual ~AsyncHttpServer(){}

        virtual void execOnRequest(const std::string& request) override;
        virtual void execOnRespond(tcp::socket& socket, std::unordered_map<std::string, std::string>& httpdata, boost::system::error_code& error) override;

        virtual bool is_async() const override {return true;}

        virtual void run() override;
        void setOnQuit(std::function<void()> f) { _onQuit = f; }

    protected : 
        std::mutex _onRequestMtx;
        std::mutex _onRespondMtx;
        ServerState _state;
        std::function<void()> _onQuit;

        const int _max_retry = 3;

        bool attemptRecovery();
};
