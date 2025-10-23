#include "network.h"
#include <iostream>
#include <iomanip>
#include "files.2/files.h"
#include "mlTime.h"
#include "exceptions.h"
#include "str.h"
#include <vector>
#include "mlprocess.h"
#include "os.h"

#ifdef _WIN32
#pragma warning(disable : 4996) // need it to exec getenv on Window
#endif

namespace network
{
    std::vector<boost::function<void()>> _onDec;
    bool _signalConnectionMade = false;

    void signalHandler(int signum)
    {
        for (auto f : _onDec)
            f();
        exit(signum);
    }

    void makeSignalConnection()
    {
        if (_signalConnectionMade)
            return;
        signal(SIGTERM, network::signalHandler);
        _signalConnectionMade = true;
    }

    void checkConnection(){std::cout << " " << std::endl;}

    void sendSSE(const std::string& event, const json& data, std::string id, int retry)
    {
        if (!event.empty())
            std::cout << "event:" << event << std::endl;
        std::cout << "data:" << data.dump() << std::endl;
        if (id.empty())
            id = "sse_" + std::to_string(ml::time::now());
        std::cout << "id:" << id << std::endl;
        if (retry>0)
        {
            std::cout << "retry:" << std::to_string(retry) << std::endl;
        }
        std::cout << std::endl;
    }

    void sendSSE(const std::string& event, const std::string& data, std::string id, int retry)
    {
        if (!event.empty())
            std::cout << "event:" << event << std::endl;
        std::cout << "data:" << data << std::endl;
        if (id.empty())
            id = "sse_" + std::to_string(ml::time::now());
        std::cout << "id:" << id << std::endl;
        if (retry>0)
        {
            std::cout << "retry:" << std::to_string(retry) << std::endl;
        }
        std::cout << std::endl;
    }

    void sendCORSHeaders(const std::string& domains )
    {
        std::cout << "Access-Control-Allow-Origin: " << domains << std::endl;
        std::cout << "Access-Control-Allow-Headers: " << domains << std::endl;
    }

    void sendSSEHeaders()
    {
        network::makeSignalConnection();
        std::cout << "Connection: keep-alive" << std::endl;
        std::cout << "Content-Type: text/event-stream" << std::endl;
        std::cout << "" << std::endl;
    }

    void sendJSONHeaders(long contentSize, bool close)
    {
        std::cout << "Content-Type: application/json" << std::endl;
        if (contentSize > 0)
            std::cout << "Content-Length: " << contentSize + 1 << std::endl;
        if (close)
            std::cout << "Connection: close" << std::endl;
        std::cout << "" << std::endl;
    }

    void sendHtmlHeaders(bool close)
    {
        std::cout << "Content-Type: html/text" << std::endl;
        if (close)
            std::cout << "Connection: close" << std::endl;
        std::cout << "" << std::endl;
    }

    std::unordered_map<std::string, std::string> get(const std::string& getstring)
    {
        std::unordered_map<std::string, std::string> _r;
        std::string _s;
        if (getstring.empty())
        {
            auto _tmp = getenv("QUERY_STRING");
            if (!_tmp)
                throw error::missing_arg("GET is not setted.");
            _s = std::string(_tmp);
        }
        else 
            _s = getstring;
        auto tmp = str::split(_s, "&");
        for (const auto& s : tmp)
        {
            auto tmp2 = str::split(s, "=");
            if (tmp2.size()<2)
                continue;
            _r[tmp2[0]] = tmp2[1];
        }
        return _r;
    }

    void addOnDeconnection(boost::function <void ()>f)
    {
        _onDec.push_back(f);
    }
}

std::string network::curl()
{
    if (os::type() == "linux")     
        return "/usr/bin/curl";
    else if (os::type() == "windows")
        return files::execDir() + files::sep() + "bin" + files::sep() + "curl.exe";
    return "curl";
}

std::string network::send(std::string url, const json& data, Method method)
{
    std::string cmd = curl();
    if (cmd != "curl" && !files::exists(cmd))
        throw std::runtime_error("The process curl : " + cmd + " don't seam to be present on the computer with this path...");
    // write the json data in a tmp file, because it could be very long and override the process buffer ...

    std::string tmppath = files::tmp() + files::sep() + "data_to_send.json";
    files::write(tmppath, data.dump());

    cmd += " -H \"Content-Type: application/json\"";
    cmd += " -k --http1.1";

    if (method == POST)
    {
        cmd += " -X POST";
        cmd += " -d \"@" + tmppath + "\"";   // this could provoque conflicts with the "'" char
    }
    else if (method == GET)
        cmd += " -X GET -G --data-urlencode \"@" + tmppath + "\""; // I have maybe broke something heere...
    cmd += " " + url;
    return process::exec(cmd);
}

std::string network::http_formated(const std::string& res,const std::string& contentType,bool cors,int serverCode)
{
    std::string r = "HTTP/1.1 " + std::to_string(serverCode) + " OK\n" + 
    "Content-type: " + contentType + "\n";
    if (cors)
    {
        r += "Access-Control-Allow-Origin: *\n";
        r += "Access-Control-Allow-Headers: *\n";
        r += "Access-Control-Allow-Methods: POST, GET, OPTIONS, DELETE, PUT\n";
    }

    r+= "\n" + res;
    return r;
}


// TODO : could add the ability to choose to add cors headers or not but useful for later, not now...
std::string network::sse_headers()
{
    std::string headers = "HTTP/1.1 200 OK\n";
    headers += "Connection: keep-alive\n";
    headers += "Content-Type: text/event-stream\n";
    headers += "Access-Control-Allow-Origin: *\n";
    headers += "Access-Control-Allow-Headers: *\n";
    headers += "Access-Control-Allow-Methods: POST, GET, OPTIONS, DELETE, PUT\n";
    headers += "\n";
    return headers;
}

std::string network::sse_formated(const std::string& event, const std::string& data, std::string id, int retry)
{
    std::string body;
    if (!event.empty())
        body += "event:" + event + std::string("\n");
    body += "data:" + data + std::string("\n");
    if (id.empty())
        id = "sse_" + std::to_string(ml::time::now());
    body += "id:" + id + std::string("\n");
    if (retry>0)
    {
        body += "retry:" + std::to_string(retry) + std::string("\n");
    }

    return body;
}

std::string network::sse_formated(const std::string& event, const json& data, std::string id, int retry)
{
    return sse_formated(event, data.dump(), id, retry);
}

std::map<std::string, std::string> default_headers()
{
    std::map<std::string, std::string> r;
    r["Host"] = "localhost";
    r["User-Agent"] = "mlapi";
    return r;
}

std::string network::url_encode(const std::string &value)
{
    // thanks chat GPT
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    for (unsigned char c : value) {
        // Unreserved characters according to RFC 3986
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int(c);
        }
    }
    return escaped.str();
}

std::string network::client_request_as_http(const std::string& path, const std::string& data, Method method, DataType dataType, std::map<std::string, std::string> headers)
{
    std::string _r;
    unsigned long _size = data.length();
    if (method ==  GET)
    {
        _r = "GET";
        if (!data.empty())
            _r += " " + path + "?" + url_encode(data) + " HTTP/1.1\r\n";
        else 
            _r += " " + path + " HTTP/1.1\r\n";
    }
    else if (method == POST)
    {
        _r = "POST";
        _r += " " + path + " HTTP/1.1\r\n";
    }

    auto def = default_headers();
    if (headers.find("Host") == headers.end())
        headers["Host"] = def["Host"];
    if (headers.find("User-Agent") == headers.end())
        headers["User-Agent"] = def["User-Agent"];
    if (dataType == JSON)
        headers["Content-Type"] = "application/json";
    if (dataType == STRING)
        headers["Content-Type"] = "text/html";

    headers["Content-Length"] = std::to_string(_size);

    for (const auto& h : headers)
        _r += h.first + ": " + h.second + "\r\n";
    _r += "\r\n";

    if (method == POST)
        _r += data;
    return _r;
}
