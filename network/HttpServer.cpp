#include "./HttpServer.h"
#include "../commands/JsonCommand.h"
#include "./network.h"
#include "./SSEEventLoop.h"
#include <memory>
#include <mutex>

HttpServer::HttpServer(int port,TcpServer::Mode mode,bool async) : TcpServer(port,mode,async)
{
    _bufSize = 8128;
    {
        std::lock_guard lk(_httpResponse);
        _httpResponse.data().contentType = "application/json";
    }

    {
        std::lock_guard lk(_onSSE);
        _onSSE.data() = 0;
    }

    auto main_cb = [this](std::unordered_map<std::string, std::string>& httpdata) -> std::string
    {
        lg("In main server callback...");
        if (httpdata.find("path") == httpdata.end())
            return "{\"success\": false, \"message\": \"No path given.\"}";

        std::string res;
        std::unordered_map<std::string, std::function<std::string(std::unordered_map<std::string, std::string>&)>> pathsfuncs;
        {
            std::lock_guard lk(_pathsFuncs);
            pathsfuncs = _pathsFuncs.data();
        }
        if (pathsfuncs.find(httpdata.at("path")) == pathsfuncs.end())
        {
            lg("Path " + httpdata.at("path") + " not found.");
            res = "{\"success\": false, \"message\": \"path " + httpdata.at("path") + " not found.\"}";
            return res;
        }

        try
        {
            res = pathsfuncs[httpdata.at("path")](httpdata);
            return res;
        }
        catch(const std::exception& e)
        {
            return "{\"success\": false, \"message\": \"" + std::string(e.what()) + "\"}";
        }
        return "{\"success\": false, \"message\": \"This should be impossible.\"}";
    };

    {
    lg("a");
        std::lock_guard lk(_httpResponses);
    lg("a");
        _httpResponses.data().push(main_cb);
    lg("a");
    }
}

HttpServer::~HttpServer(){}

void HttpServer::handleSocket(std::shared_ptr<tcp::socket> s)
{
    lg("handling the socket ...");
    std::string headers = this->readSocket(s, _bufSize, "\r\n\r\n");
    std::unordered_map<std::string, std::string> httpdata;
    httpdata = network::httpHeaders(headers);

    if (httpdata.find("Content-Length") != httpdata.end())
    {
        auto length = strtoll(httpdata["Content-Length"].c_str(), nullptr, 10);
        if (length > 0)
        {
            auto realHeaderSize = str::split(headers, "\r\n\r\n")[0].size() + 4;// 4 is the size of \r\n\r\n
            long long lengthToRead = (long long)(length + realHeaderSize) - (long long)headers.size();
            if (lengthToRead > 0)
            {
                auto rest = this->readSocket(s, (size_t)lengthToRead);
                httpdata["content"] += rest;
            }

            if (httpdata["content"].size() != length)
            {
                _execOnError("Error in reading http request : " + std::to_string(httpdata["content"].size()) + " != " + std::to_string(length));
                return;
            }
        }
    }

    {
        std::lock_guard lk(_onEveryRequest);
        for (const auto& f : _onEveryRequest.data())
            f();
    }

    if (httpdata.find("path") != httpdata.end() && httpdata["path"] == "/sse")
    {
        bool isSSELoop = false;
        {
            std::lock_guard lk(_sseUniqueLoop);
            if (_sseUniqueLoop.data())
                isSSELoop = true;
        }
        if (!isSSELoop)
        {
            std::function<void(std::shared_ptr<tcp::socket>, std::unordered_map<std::string, std::string>&)> onsse = 0;
            {
                std::lock_guard lk(_onSSE);
                onsse = _onSSE.data();
            }

            if (onsse)
            {
                auto hds = network::sse_headers();
                this->write(s, hds);
                onsse(s, httpdata);
            }
        }
        else 
            _sseUniqueLoop.data(true)->addSubscriber(s);
        return;
    }

    ml::Vec<std::function<std::string(std::unordered_map<std::string, std::string>&)>> httpresponses;
    {
        std::lock_guard lk(_httpResponses);
        httpresponses = _httpResponses.data();
    }

    HttpResponse httpresponse;
    {
        std::lock_guard lk(_httpResponse);
        httpresponse = _httpResponse.data();
    }

    for (const auto& f : httpresponses)
    {
        auto res = f(httpdata);
        res = network::http_formated(res, httpresponse.contentType, httpresponse.cors, httpresponse.code);
        res += "\n";
        this->write(s, res);
    }
}

std::string HttpServer::readSocket(std::shared_ptr<tcp::socket> s, size_t length, const std::string& separator)
{
    std::string readed;
    readed.reserve(length); // Pre-allocate memory

    std::vector<char> buf(std::min(static_cast<size_t>(_bufSize), length));

    boost::system::error_code ec;
    while (readed.size() < length && !ec) 
    {
        size_t len = s->read_some(boost::asio::buffer(buf), ec);

        if (ec) 
        {
            if (ec != boost::asio::error::eof) 
                _execOnError(ec.message());
        }

        readed.append(buf.data(), len);

        if (!separator.empty() && readed.find(separator) != std::string::npos) 
            break;
    }

    return readed;
}

json HttpServer::jsonBody(std::unordered_map<std::string, std::string>& httpdata)
{
    if (httpdata.find("content") == httpdata.end())
        return json::object();
    json j;
    try
    {
        j = json::parse(httpdata["content"]);
    }
    catch(const std::exception& e){}
    return j;
}

std::string HttpServer::success( json& data) const
{
    data["success"] = true;
    return json(data).dump();
}

std::string HttpServer::success(const std::string& msg) const
{
    json j;
    j["success"] = true;
    j["message"] = msg;
    return json(j).dump();
}

std::string HttpServer::failure( json& data) const
{
    data["success"] = false;
    return json(data).dump();
}

std::string HttpServer::failure(const std::string& msg) const
{
    json j;
    j["success"] = false;
    j["message"] = msg;
    return json(j).dump();
}

void HttpServer::successCmd(std::shared_ptr<JsonCommand> cmd, const std::string& message) const
{
    json& j = cmd->returnValue();
    j["message"] = message;
    j["success"] = true;
}

void HttpServer::failureCmd(std::shared_ptr<JsonCommand> cmd, const std::string& message) const
{
    json& j = cmd->returnValue();
    j["data"] = {};
    j["message"] = message;
    j["success"] = false;
}

void HttpServer::successCmdJson(std::shared_ptr<JsonCommand> cmd, const json& j) const
{
    json& _new = cmd->returnValue();
    _new["success"] = true;
    _new["data"] = j;
    _new["message"] = "";
}
void HttpServer::failureCmdJson(std::shared_ptr<JsonCommand> cmd, const json& j) const
{
    json& _new = cmd->returnValue();
    _new["success"] = false;
    _new["data"] = j;
    _new["message"] = "";
}

void HttpServer::successCmd(JsonCommand& cmd,const std::string& message) const
{
    json& j = cmd.returnValue();
    j["message"] = message;
    j["success"] = true;
}

void HttpServer::failureCmd(JsonCommand& cmd,const std::string& message) const
{
    json& j = cmd.returnValue();
    j["data"] = {};
    j["message"] = message;
    j["success"] = false;
}

void HttpServer::successCmdJson(JsonCommand& cmd,const json& j) const
{
    json& _new = cmd.returnValue();
    _new["success"] = true;
    _new["data"] = j;
    _new["message"] = "";
}

void HttpServer::failureCmdJson(JsonCommand& cmd,const json& j) const
{
	json& _new = cmd.returnValue();
    _new["success"] = false;
    _new["data"] = j;
    _new["message"] = "";
}

void HttpServer::createJsonCommand(const std::string& path, 
                const std::function<void(JsonCommand&)>& func, 
                const std::vector<std::string>& mendatoryKeys)
{
    lg("Creating command " + path);
    std::lock_guard lk(_cmds);
    auto cmd = _cmds.data().createCommand<JsonCommand>(path); 	
    cmd->setMendatoryKeys(mendatoryKeys);
    auto f = [this, path, cmd, func] (const json& args) mutable -> std::string
    {
        lg("Executing path from " + path);
        JsonCommand cmd_cp;
        {
            std::lock_guard lk(_cmds);
            cmd_cp = *cmd;
        }
        cmd_cp.setCmdExec(func);
        try
        {
            cmd_cp.setJsonArgs(args);
            cmd_cp.exec();
            {
                std::lock_guard lk(_onEveryJsonCommands);
                for(const auto& f : _onEveryJsonCommands.data())
                    f(cmd_cp);
            }
            return cmd_cp.returnString() + "\n";
        }
        catch(const std::exception& e)
        {
            lg(e.what());
            return "{\"success\": false, \"message\": \"" + (std::string)e.what() + "\"}";
        }
    };

    this->addJsonFuncByPath(path,f);
}

void HttpServer::addFuncByPath(std::string path, const std::function<std::string(std::unordered_map<std::string, std::string>&)>& func)
{
    std::lock_guard lk(_pathsFuncs);
    _pathsFuncs.data()[path] = [func](std::unordered_map<std::string, std::string>& data) -> std::string
    {
        try
        {
            return func(data);
        }
        catch(const std::exception& e)
        {
            return "{\"success\": false, \"message\": \"" + (std::string)e.what() + "\"}";
        }
    };
}

void HttpServer::addFuncByPath(std::string path, const std::function<std::string()>& func)
{
    std::lock_guard lk(_pathsFuncs);
    _pathsFuncs.data()[path] = [func](std::unordered_map<std::string, std::string>& data) -> std::string
    {
        try
        {
            return func();
        }
        catch(const std::exception& e)
        {
            return "{\"success\": false, \"message\": \"" + (std::string)e.what() + "\"}";
        }
    };
}

void HttpServer::addJsonFuncByPath(std::string path, const std::function<std::string(const json&)>& func)
{
    std::lock_guard lk(_pathsFuncs);
    _pathsFuncs.data()[path] = [this, func](std::unordered_map<std::string, std::string>& data) -> std::string
    {
        try
        {
            return func(this->jsonBody(data));
        }
        catch(const std::exception& e)
        {
            return "{\"success\": false, \"message\": \"" + (std::string)e.what() + "\"}";
        }
    };
}

std::string HttpServer::root(std::unordered_map<std::string, std::string>& httpdata)
{
    if (httpdata.find("path") == httpdata.end())
        return "";
    return httpdata["path"];
}

void HttpServer::setOnSSE(const std::function<void(std::shared_ptr<tcp::socket>, std::unordered_map<std::string, std::string>&)>& func)
{
    std::lock_guard lk(_onSSE);
    _onSSE.data() = func;
}

bool HttpServer::sendAsSSE(std::shared_ptr<tcp::socket> socket,const json& data)
{
    boost::system::error_code ec;

    auto s = network::sse_formated("message", data, "");
    s+= "\n";
    this->write(socket, s, ec);

    return !(ec == boost::asio::error::eof || 
            ec == boost::asio::error::connection_reset ||
            ec == boost::asio::error::broken_pipe);
}

bool HttpServer::sendAsSSE(std::shared_ptr<tcp::socket> socket,const std::string& data)
{
    boost::system::error_code ec;
    auto s = network::sse_formated("message", data, "");
    s+= "\n";
    this->write(socket, s, ec);
    return !(ec == boost::asio::error::eof || 
            ec == boost::asio::error::connection_reset ||
            ec == boost::asio::error::broken_pipe);
}

void HttpServer::addOnEveryRequest(const std::function<void()>& func)
{
    std::lock_guard lk(_onEveryRequest);
    _onEveryRequest.data().push_back(func);
}

void HttpServer::addOnEveryJsonCommands(const std::function<void(JsonCommand&)>& func)
{
    std::lock_guard lk(_onEveryJsonCommands);
    _onEveryJsonCommands.data().push_back(func);
}

void HttpServer::createUniqueSSELoop()
{
    std::lock_guard lk(_sseUniqueLoop);
    _sseUniqueLoop.data() = std::make_unique<SSEEventLoop>(this);
    auto run = [this]{
        _sseUniqueLoop.data(true)->run();
    };
    _pool.run(run);
}
