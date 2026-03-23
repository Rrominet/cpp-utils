#include "./HttpServer.h"
#include "network/network.h"
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
    };

    {
        std::lock_guard lk(_httpResponses);
        _httpResponses.data().push(main_cb);
    }
}

HttpServer::~HttpServer(){}

void HttpServer::handleSocket(std::shared_ptr<tcp::socket> s)
{
    std::string headers = this->readSocket(s, _bufSize, "\r\n\r\n");
    std::unordered_map<std::string, std::string> httpdata;
    httpdata = network::httpHeaders(headers);

    if (httpdata.find("Content-Length") != httpdata.end())
    {
        auto length = strtoll(httpdata["Content-Length"].c_str(), nullptr, 10);
        if (length > 0)
        {
            auto realHeaderSize = str::split(headers, "\r\n\r\n")[0].size() + 4;// 4 is the size of \r\n\r\n
            auto lengthToRead = length + realHeaderSize; // because header size not in Content-Length
            lengthToRead -= headers.size();
            if (lengthToRead > 0)
            {
                auto rest = this->readSocket(s, lengthToRead);
                httpdata["content"] += rest;
            }

            if (httpdata["content"].size() != length)
            {
                _execOnError("Error in reading http request : " + std::to_string(httpdata["content"].size()) + " != " + std::to_string(length));
                return;
            }
        }
    }

    if (httpdata.find("path") != httpdata.end() && httpdata["path"] == "/sse")
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

std::shared_ptr<JsonCommand> HttpServer::createJsonCommand(const std::string& path)
{
    std::lock_guard lk(_cmds);
    auto cmd = _cmds.data().createCommand<JsonCommand>(path); 	
    auto f = [this, path, cmd] (const json& httpdata) mutable -> std::string
    {
        JsonCommand cmd_cp;
        {
            std::lock_guard lk(_cmds);
            cmd_cp = *cmd;
        }
        try
        {
            if (httpdata.contains("method") && httpdata["method"] == "OPTIONS")
                return "";
            if (httpdata.contains("content"))
                cmd_cp.setJsonArgs(httpdata["content"]);
            else 
                cmd_cp.setJsonArgs(json::object());
            cmd_cp.exec();
            return cmd_cp.returnString() + "\n";
        }
        catch(const std::exception& e)
        {
            lg(e.what());
            return "{\"success\": false, \"message\": \"" + (std::string)e.what() + "\"}";
        }
    };

    this->addJsonFuncByPath(path,f);
    return cmd;

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
    this->write(socket, s);
    return !(ec == boost::asio::error::eof || 
            ec == boost::asio::error::connection_reset ||
            ec == boost::asio::error::broken_pipe);
}
