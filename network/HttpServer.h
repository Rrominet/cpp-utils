#pragma once
#include "TcpServer.h"
#include <unordered_map>
#include <boost/beast/core.hpp>
#include <mutex>
#include "network.h"
#include "vec.h"
#include <nlohmann/json.hpp>
#include "../commands/CommandsManager.h"
#include "../commands/JsonCommand.h"
using json = nlohmann::json;
//
// To add a function to the server, use the addFuncByPath function.
// The path is simply the path in the URL.
// The function will be executed asyncronly when the path is requested. 
// This unorderd_map is the httpdata from the request giving all infos on it you could want.
// The function should return the string that will be sended to the client
//
// By default, all the functions should be implemented in setFuncsByPaths() but you can add new ones anywhere. If you're not sure, just add them at the end of setFuncsByPaths()
// If an exception in thrown in your function, the server will send a json with {"success": false, "message": "error message from the exception"}
//
// That's it, good luck ! :)
// this server is made for HTTP/1.1 requests.
//
// for server-side events, you need to set the _onSSE function (with setOnSSE...)
// in it, you can use the sendSSE function to send events to the client.
// your responsible to keep a loop opened in it, if not the connexion will be closed.
// in a HttpServer, the sse if looped, will block any other request.
// in an AsyncHttpServer, ths sse if looped run on its own thread so the other requests will not be blocked.
// Here is a simple working example of an sse loop:
//    auto sse = [this](auto& s, auto& httpdata)
//     {
//         while(true)
//         {
//             std::this_thread::sleep_for(std::chrono::seconds(1));
//             json d;
//             d["time"] = std::chrono::system_clock::now().time_since_epoch().count();
//             d["httpdata_received"] = httpdata;
//             this->sendAsSSE(s, d);
//         }
//     };
//     this->setOnSSE(sse);
//     A client can listen to sse simply by calling the route /sse

struct HttpResponse
{
    int code = 200;
    std::string contentType = "text/html";
    bool cors = true;
};

class HttpServer : public TcpServer
{
    protected : 
        HttpResponse _httpResponse;
        std::mutex _socketMtx;
        std::unordered_map<std::string, std::function<std::string(std::unordered_map<std::string, std::string>&)>> _pathsFuncs;
        std::function<void(tcp::socket&, std::unordered_map<std::string, std::string>&)> _onSSE = 0;

        ml::CommandsManager _cmds; //bp cg

    public : 
        // executed after receiving a request just before sending the response the response body will be the returned std::string;
        ml::Vec<std::function<std::string(std::unordered_map<std::string, std::string>)>> httpResponses;

        HttpServer(int port=8080, Mode mode=IP4);

        template<typename Server>
        static Server* create(int argc, char *argv[])
        {
            std::string error;
            if (argc < 2)
            {
                error += "Usage: " + std::string(argv[0]) + " <port>\n";
                error += "No port given.";
                throw std::invalid_argument(error);
            }

            int port = 0;
            try
            {
                port = std::stoi(argv[1]);
            }
            catch(const std::exception& e)
            {
                error += "Failed to convert the port to a integer...\n";
                error += "Port given " + std::string(argv[1]) + "\n";
                error += "Error : " + std::string(e.what()) + "\n";
                throw std::invalid_argument(error);
            }
            return new Server(port);
        }

        void _construct();
        virtual ~HttpServer(){}

        std::unordered_map<std::string, std::string> parsedHeaders(std::string requestBegining);

        std::string readSocket(tcp::socket& socket, long length, boost::system::error_code& error, const std::string separator="");
        void write(tcp::socket& socket, const std::string& res, boost::system::error_code& error);
        void write(tcp::socket& socket, const std::string& res);

        std::string request(std::unordered_map<std::string, std::string>& httpdata);

        HttpResponse& httpResponse(){return _httpResponse;}

        virtual std::string formatRes(const std::string& res) const override;

        virtual void run() override;
        std::string root(std::unordered_map<std::string, std::string>& httpdata);

        virtual void execOnRequest(const std::string& request);
        virtual void execAllCallbacks(tcp::socket* s, const std::string& request, std::unordered_map<std::string, std::string>& httpdata);

        virtual void execOnRespond(tcp::socket& socket, std::unordered_map<std::string, std::string>& httpdata, boost::system::error_code& error);

        virtual bool is_async() const {return false;}

        // need to be reimplemented in your child class for your server to actually do something
        // you can add a path reaction by using addFuncByPath
        // you need to call it in your constructor to (or equivalent), it is not called by default.
        virtual void setFuncsByPaths(){}
        virtual void addFuncByPath(std::string path, const std::function<std::string(std::unordered_map<std::string, std::string>&)>& func);
        virtual void addFuncByPath(std::string path, const std::function<std::string()>& func);
        virtual void addJsonFuncByPath(std::string path, const std::function<std::string(const json&)>& func);

        json jsonBody(std::unordered_map<std::string, std::string>& httpdata);

        // useful to craft quick awnser !
        std::string success( json& data) const;
        std::string success(const std::string& msg) const;

        std::string failure( json& data) const;
        std::string failure(const std::string& msg) const;

        // exec one requestLoop (in run)
        // separated to be async in the AsyncHttpServer version (in run)
        void onRequestLoop(tcp::socket& s, long bufSize);

        void successCmd(std::shared_ptr<JsonCommand> cmd, const std::string& message="") const;
        void failureCmd(std::shared_ptr<JsonCommand> cmd, const std::string& message="") const;

        void successCmdJson(std::shared_ptr<JsonCommand> cmd, const json& j) const;
        void failureCmdJson(std::shared_ptr<JsonCommand> cmd, const json& j) const;

        std::shared_ptr<JsonCommand> createJsonCommand(const std::string& path);

        void setOnSSE(const std::function<void(tcp::socket&, std::unordered_map<std::string, std::string>&)>& func) {_onSSE = func;}
        void sendAsSSE(tcp::socket& socket, const json& data);
        void sendAsSSE(tcp::socket& socket, const std::string& data);

    public : 
#include "./HttpServer_gen.h"
};
