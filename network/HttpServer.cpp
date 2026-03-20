#include "./HttpServer.h"
#include "network/network.h"

HttpServer::HttpServer(int port,TcpServer::Mode mode,bool async) : TcpServer(port,mode,async)
{
    _bufSize = 8128;
    _httpResponse.contentType = "application/json";

    auto main_cb = [this](std::unordered_map<std::string, std::string>& httpdata) -> std::string
    {
        if (httpdata.find("path") == httpdata.end())
            return "{\"success\": false, \"message\": \"No path given.\"}";

        std::string res;
        if (_pathsFuncs.find(httpdata.at("path")) == _pathsFuncs.end())
        {
            res = "{\"success\": false, \"message\": \"path " + httpdata.at("path") + " not found.\"}";
            return res;
        }

        try
        {
            res = _pathsFuncs[httpdata.at("path")](httpdata);
            return res;
        }
        catch(const std::exception& e)
        {
            return "{\"success\": false, \"message\": \"" + std::string(e.what()) + "\"}";
        }
    };
    _httpResponses.push(main_cb);
}

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

    for (const auto& f : _httpResponses)
    {
        auto res = f(httpdata);
        res = network::http_formated(res, _httpResponse.contentType, _httpResponse.cors, _httpResponse.code);
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
    auto cmd = _cmds.createCommand<JsonCommand>(path); 	
    this->addJsonFuncByPath(path, [this, path, cmd] (const json& httpdata) mutable -> std::string
            {
                if (httpdata["method"] == "OPTIONS")
                    return "";
                cmd->setJsonArgs(httpdata["content"]);
                cmd->exec();
                return cmd->returnString() + "\n";
            });
    return cmd;
}

void HttpServer::addFuncByPath(std::string path, const std::function<std::string(std::unordered_map<std::string, std::string>&)>& func)
{
    _pathsFuncs[path] = [func](std::unordered_map<std::string, std::string>& data) -> std::string
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
    _pathsFuncs[path] = [func](std::unordered_map<std::string, std::string>& data) -> std::string
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
    _pathsFuncs[path] = [this, func](std::unordered_map<std::string, std::string>& data) -> std::string
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

