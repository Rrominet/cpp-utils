#include "HttpServer.h"
#include "str.h"
#include "uri.h"
#include "./network.h"
#include <mutex>

HttpServer::HttpServer(int port, Mode mode) : TcpServer(port, mode)
{
    _construct();
}

void HttpServer::_construct()
{
    _bufSize = 8128;
    _httpResponse.contentType = "application/json";
    this->httpResponses.push([this](std::unordered_map<std::string, std::string> data) -> std::string
            {
                db::write("Received a requets for path " + data["path"]);
                std::string res;
                if (_pathsFuncs.find(data["path"]) == _pathsFuncs.end())
                {
                    res = "{\"success\": false, \"message\": \"path " + data["path"] + " not found.\"}";
                    db::write("path " + data["path"] + " not found\nResponding : " + res);
                    return res;
                }

                db::write("path " + data["path"] + " found.");
                res = _pathsFuncs[data["path"]](data);
                db::write("Responding : " + res);
                return res;
            });
}

void HttpServer::run()
{
    try
    {
        for (;;)
        {
            tcp::socket socket(_io_service);
            _acceptor->accept(socket);
            this->onRequestLoop(socket, _bufSize);
        }
    }

    catch(const std::exception& e){lg("Error in the server loop : " << e.what()); throw;}
}

void HttpServer::execOnRequest(const std::string& request)
{
    for (auto &f : _onRequest)
        f(request);
}

void HttpServer::execAllCallbacks(tcp::socket* s, const std::string& request, std::unordered_map<std::string, std::string>& httpdata)
{
    this->execOnRequest(request);
    boost::system::error_code error;
    this->execOnRespond(*s, httpdata, error);
}

std::unordered_map<std::string, std::string> HttpServer::parsedHeaders(std::string requestBegining)
{
    if (!str::contains(requestBegining, "HTTP/"))
        throw std::runtime_error("Not an HTTP request");

    std::unordered_map<std::string, std::string> _r;
    auto tmp = str::split(requestBegining, "\r\n\r\n");

    if (tmp.size()>1 && !tmp[1].empty())
        _r["content"] = tmp[1];

    std::string headers = tmp[0];
    db_write2("headers", headers);

    tmp = str::split(headers, "\r\n");
    auto tmp2 = str::split(tmp[0], " ");
    _r["method"] = tmp2[0];
    _r["protocol"] = tmp2[2];

    auto tmp3 = str::split(tmp2[1], "?");
    _r["path"] = uri::decode(tmp3[0]);
    if (tmp3.size()>1)
    {
        _r["get"] = uri::decode(tmp3[1]);
        if (_r.find("content") == _r.end())
            _r["content"] = _r["get"];
    }

    db_write2("method", _r["method"]);
    db_write2("protocol", _r["protocol"]);
    db_write2("path", _r["path"]);
    db_write2("content", _r["content"]);

    for (int i=1; i<tmp.size(); i++)
    {
        std::string key;
        std::string val;
        bool isKey = true;
        for (int j=0; j<tmp[i].size(); j++)
        {
            if (j<tmp[i].size()-1)
            {
                if (tmp[i][j] == ':' && tmp[i][j+1] == ' ')
                {
                    isKey = false;
                    continue;
                }
                if (i>0 && tmp[i][j] == ' ' && tmp[i][j-1] == ':')
                    continue;
            }

            if (isKey)
                key.push_back(tmp[i][j]);
            else
                val.push_back(tmp[i][j]);
        }

        db_write2("Key", key);
        db_write2("Val", val);
        if (!key.empty())
            _r[key] = val;
    }

    return _r;
}

std::string HttpServer::request(std::unordered_map<std::string, std::string>& httpdata)
{
    if (httpdata.find("content") != httpdata.end())
        return httpdata["content"];
    return "";
}

std::string HttpServer::formatRes(const std::string& res) const
{
    return network::http_formated(res, _httpResponse.contentType, _httpResponse.cors, _httpResponse.code);
}

std::string HttpServer::root(std::unordered_map<std::string, std::string>& httpdata)
{
    try
    {
        return httpdata["path"];
    }
    catch(...)
    {
        lg("Error  : can't return path from httpdata");
        return "";
    }
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

json HttpServer::jsonBody(std::unordered_map<std::string,std::string>& httpdata)
{
    if (httpdata.find("content") == httpdata.end())
        throw std::runtime_error("No content given.");
    try
    {
        return json::parse(httpdata["content"]);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("This is not a JSON.");
    }
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

void HttpServer::onRequestLoop(tcp::socket& s, long bufSize)
{
    boost::system::error_code error;
    std::string headers;
    try
    {
        headers = this->readSocket(s, _bufSize, error, "\r\n\r\n");

        lg2(headers.size(), "bytes read");
        lg2(_bufSize, "asked to read");
        assert(headers.size() <= bufSize);
    }
    catch(const std::exception& e)
    {
        db_write("Error in readAllSockets : " + (std::string)e.what()); 
        return;
    }

    db_write("RequestBegining sended : " + headers);
    std::unordered_map<std::string, std::string> httpdata;
    try
    {
        httpdata = this->parsedHeaders(headers);
    }
    catch(const std::exception& e)
    {
        db_write2("Error in HTTP Header parsing", e.what());
        return;
    }

    auto length = strtoll(httpdata["Content-Length"].c_str(), nullptr, 10);
    db_write("request body length : " + std::to_string(length));
    std::string request;
    if (length == 0)
        request = headers;
    else 
    {
        lg("Request length is not 0, so we read more !");
        auto realHeaderSize = str::split(headers, "\r\n\r\n")[0].size() + 4; // 4 is the size of \r\n\r\n
        auto lengthToRead = length + realHeaderSize; // because header size not in Content-Length
        lengthToRead -= headers.size();
        if (lengthToRead > 0)
        {
            auto rest = this->readSocket(s, lengthToRead, error);
            db_write2("reste readed", rest.size());
            httpdata["content"] += rest;
            db_write2("total content in http_data", httpdata["content"].size());
        }
        lg2("real headers size", realHeaderSize);
        lg2("Content-Length", length);
        lg2("http_data[content]", httpdata["content"].size());
        request = headers + "\r\n\r\n" + httpdata["content"];
        assert(httpdata["content"].size() == length);
    }

    if (httpdata["path"] == "/sse" && _onSSE)
    {
        auto hds = network::sse_headers();
        this->write(s, hds, error);
        _onSSE(s, httpdata);
    }
    else 
        this->execAllCallbacks(&s, request, httpdata);
}

void HttpServer::execOnRespond(tcp::socket& socket, std::unordered_map<std::string, std::string>& httpdata, boost::system::error_code& error)
{
    for (auto &f : _onRespond)
    {
        auto res = f(this->request(httpdata));
        res = this->formatRes(res);
        res += "\n";
        this->write(socket, res, error);
    }

    for (auto &f : httpResponses)
    {
        auto res = f(httpdata);
        res = this->formatRes(res);
        res += "\n";
        this->write(socket, res, error);
    }
}

void HttpServer::write(tcp::socket& socket, const std::string& res, boost::system::error_code& error)
{
    for (size_t j=0; j<res.size(); j+=4096)
    {
        std::string _res = res.substr(j, 4096);
        boost::asio::write(socket, boost::asio::buffer(_res), error);
    }
}
void HttpServer::write(tcp::socket& socket, const std::string& res)
{
    for (size_t j=0; j<res.size(); j+=4096)
    {
        std::string _res = res.substr(j, 4096);
        boost::asio::write(socket, boost::asio::buffer(_res));
    }
}

std::string HttpServer::readSocket(tcp::socket& socket, long length, boost::system::error_code& error, const std::string separator)
{
    const size_t CHUNK_SIZE = 4096;
    std::string final_request;
    final_request.reserve(length); // Pre-allocate memory
    
    std::vector<char> buf(std::min(CHUNK_SIZE, static_cast<size_t>(length)));
    
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    
    std::unique_lock<std::mutex> lock(_socketMtx);
    while (final_request.size() < length) 
    {
        if (std::chrono::steady_clock::now() > deadline) 
            throw std::runtime_error("Socket read timeout");
        
        boost::system::error_code ec;
        size_t len = socket.read_some(boost::asio::buffer(buf), ec);
        
        if (ec) 
        {
            if (ec == boost::asio::error::eof) 
                break;
            throw boost::system::system_error(ec);
        }
        
        final_request.append(buf.data(), len);
        
        if (!separator.empty() && final_request.find(separator) != std::string::npos) 
            break;
    }
    
    return final_request;
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
                lg(httpdata);
                if (httpdata["method"] == "OPTIONS")
                    return "";
                cmd->setJsonArgs(httpdata["content"]);
                cmd->exec();
                return cmd->returnString() + "\n";
            });
    return cmd;
}

void HttpServer::sendAsSSE(tcp::socket& socket, const json& data)
{
    boost::system::error_code error;
    auto s = network::sse_formated("message", data, "");
    s+= "\n";
    this->write(socket, s, error);
}

void HttpServer::sendAsSSE(tcp::socket& socket, const std::string& data)
{
    boost::system::error_code error;
    auto s = network::sse_formated("message", data, "");
    s+= "\n";
    this->write(socket, s, error);
}

