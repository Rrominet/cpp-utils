#pragma once
#include "./TcpServer.h"
#include "../vec.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../commands/CommandsManager.h"
#include "../commands/JsonCommand.h"
#include "../Ret.h"
//
// To add a function to the server, use the addFuncByPath function.
// The path is simply the path in the URL.
// The function will be executed asyncronly when the path is requested (when _async == true). 
// This unorderd_map is the httpdata from the request giving all infos on it you could want.
// The function should return the string that will be sended to the client
//
// If an exception in thrown in your function, the server will send a json with {"success": false, "message": "error message from the exception"}
//
// Use the createJsonCommand function to create a JsonCommand that will be automaticly executed on the good path with the copied command an the execution thread as argument (it calls addFuncByPath inside.)
// createjsonCommand is certainly the best way to add a command, you want to use that the most.
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
//     auto sse = [this](std::shared_ptr<tcp::socket> s, std::unordered_map<std::string, std::string>& data)
//     {
//         while(true)
//         {
//             std::this_thread::sleep_for(std::chrono::seconds(1));
//             json d;
//             d["time"] = std::chrono::system_clock::now().time_since_epoch().count();
//             d["httpdata_received"] = data;
//             bool alive = this->sendAsSSE(s, d);
//             if (!alive)
//                 break;
//         }
//     };
// 
//     this->setOnSSE(sse);
//     A client can listen to sse simply by calling the route /sse
//     //TODO : add a set of different paths for sse in a map
//     Warning : Don't forget to kill your loop when the client is not alive anymore because eash on _sse function is executed PER CLIENT. If not you will have a server that runs thousands of sse function that runs on dead clients...


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
        virtual ~HttpServer();

        //All the real work is happenning here : 
        //------------------------------------
        virtual void handleSocket(std::shared_ptr<tcp::socket> s) override;
        std::string readSocket(std::shared_ptr<tcp::socket> s, size_t length, const std::string& separator="\r\n\r\n");
        //------------------------------------

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

        void successCmd(JsonCommand& cmd, const std::string& message="") const;
        void failureCmd(JsonCommand& cmd, const std::string& message="") const;

        void successCmdJson(JsonCommand& cmd, const json& j) const;
        void failureCmdJson(JsonCommand& cmd, const json& j) const;

        void createJsonCommand(const std::string& path, 
                const std::function<void(JsonCommand&)>& func, 
                const std::vector<std::string>& mendatoryKeys = std::vector<std::string>());

        void addFuncByPath(std::string path, const std::function<std::string(std::unordered_map<std::string, std::string>&)>& func);
        void addFuncByPath(std::string path, const std::function<std::string()>& func);
        void addJsonFuncByPath(std::string path, const std::function<std::string(const json&)>& func);

        std::string root(std::unordered_map<std::string, std::string>& httpdata);
        void setOnSSE(const std::function<void(std::shared_ptr<tcp::socket>, std::unordered_map<std::string, std::string>&)>& func);

        //return false if the client socket is dead
        bool sendAsSSE(std::shared_ptr<tcp::socket> socket, const json& data);
        bool sendAsSSE(std::shared_ptr<tcp::socket>, const std::string& data);

    protected : 
        th::Safe<ml::Vec<std::function<std::string(std::unordered_map<std::string, std::string>&)>>> _httpResponses;
        th::Safe<std::unordered_map<std::string, std::function<std::string(std::unordered_map<std::string, std::string>&)>>> _pathsFuncs;
        th::Safe<HttpResponse> _httpResponse;
        th::Safe<ml::CommandsManager> _cmds; //bp cg

        th::Safe<std::function<void(std::shared_ptr<tcp::socket>, std::unordered_map<std::string, std::string>&)>> _onSSE;
};

