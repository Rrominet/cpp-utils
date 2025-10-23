#pragma once
#include "mlprocess.h"
#include <map>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace http
{
    class Request : public Process
    {
        public : 
            Request(const std::string& cmd="") : Process(cmd){}
            virtual ~Request(){}
    };

    enum Method
    {
        GET = 1, 
        POST = 2,
        _DELETE = 3,
    };

    extern std::string curl; // you can change it in your program if you want, it's curl absolute path.

    std::string urlEncoded(const std::map<std::string, std::string> &params);
    std::string urlEncoded(std::string data);

    std::string cmd(std::string url, 
            const std::string& sdata="", 
            const std::map<std::string, std::string> &params = {},
            json data={}, 
            Method method=GET,
            ml::Vec<std::string> headers = {});

    void request(std::string url, 
            const std::string& sdata="", 
            const std::map<std::string, std::string> &params = {},
            json data={}, 
            Method method=GET,
            boost::function<void (std::string)> onDoned=0, 
            boost::function<void (std::string)> onPgr=0, 
            boost::function<void (std::string)> onError=0);

    void get(const std::string& url,
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr=0, 
            boost::function<void (std::string)> onError=0);

    void get_sync(const std::string& url);

    void get(const std::string& url, const std::map<std::string, std::string> &params, 
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr=0, 
            boost::function<void (std::string)> onError=0);

    void post(const std::string& url, std::map<std::string, std::string> params, 
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr=0, 
            boost::function<void (std::string)> onError=0);

    void post(const std::string& url, json data, 
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr=0, 
            boost::function<void (std::string)> onError=0);

    void post(const std::string& url, const std::string& data, 
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr=0, 
            boost::function<void (std::string)> onError=0);
    void test();

    std::string get(const std::string& url, ml::Vec<std::string> headers = {});
    std::string get(const std::string& url, const std::map<std::string, std::string> &params, ml::Vec<std::string> headers = {});

    std::string post(const std::string& url, std::map<std::string, std::string> params, ml::Vec<std::string> headers = {});
    std::string post(const std::string& url, json data, ml::Vec<std::string> headers = {});
    std::string post(const std::string& url, const std::string& data, ml::Vec<std::string> headers = {});
}
