#pragma once
#include "./TcpServer.h"
#include "../vec.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../commands/CommandsManager.h"
#include "../commands/JsonCommand.h"
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
// Use the createJsonCommand function to create a JsonCommand that will be automaticly executed on the good path with the json body request as argument (it calls addFuncByPath inside.)
// createjsonCommand is certainly the onw you want to use the most.
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


//TODO : implement SSE
struct HttpResponse
{
    int code = 200;
    std::string contentType = "text/html";
    bool cors = true;
};

class HttpServer : public TcpServer
{
    public:
        HttpServer(int port, TcpServer::Mode mode, bool async);
        virtual ~HttpServer() = default;

        //All the real work is happenning here : 
        //------------------------------------
        virtual void handleSocket(std::shared_ptr<tcp::socket> s) override;
        std::string readSocket(std::shared_ptr<tcp::socket> s, size_t length, const std::string& separator="\r\n\r\n");
        //------------------------------------

        //change this before accepting requests.
        //not during request processing if the server is async.
        HttpResponse& httpResponse() { return _httpResponse; }

        json jsonBody(std::unordered_map<std::string, std::string>& httpdata);

        // useful to craft quick awnser !
        std::string success( json& data) const;
        std::string success(const std::string& msg) const;

        std::string failure( json& data) const;
        std::string failure(const std::string& msg) const;

        void successCmd(std::shared_ptr<JsonCommand> cmd, const std::string& message="") const;
        void failureCmd(std::shared_ptr<JsonCommand> cmd, const std::string& message="") const;

        void successCmdJson(std::shared_ptr<JsonCommand> cmd, const json& j) const;
        void failureCmdJson(std::shared_ptr<JsonCommand> cmd, const json& j) const;
        std::shared_ptr<JsonCommand> createJsonCommand(const std::string& path);

        void addFuncByPath(std::string path, const std::function<std::string(std::unordered_map<std::string, std::string>&)>& func);
        void addFuncByPath(std::string path, const std::function<std::string()>& func);
        void addJsonFuncByPath(std::string path, const std::function<std::string(const json&)>& func);

        std::string root(std::unordered_map<std::string, std::string>& httpdata);

        //TODO: implement
//         void setOnSSE(const std::function<void(tcp::socket&, std::unordered_map<std::string, std::string>&)>& func) {_onSSE = func;}
//         void sendAsSSE(tcp::socket& socket, const json& data);
//         void sendAsSSE(tcp::socket& socket, const std::string& data);

    protected : 
        ml::Vec<std::function<std::string(std::unordered_map<std::string, std::string>&)>> _httpResponses;
        std::unordered_map<std::string, std::function<std::string(std::unordered_map<std::string, std::string>&)>> _pathsFuncs;
        HttpResponse _httpResponse;
        ml::CommandsManager _cmds; //bp cg
};

