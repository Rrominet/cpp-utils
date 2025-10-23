#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "str.h"
#include "thread.h"
#include <type_traits>
#include "debug.h"
#include <csignal>
#include <boost/function.hpp>

namespace network
{
    //send a server-side-request
    //this will kill the cgi script if the connection is closed by the client --> killed attemting to cout so there is no risks of data corruption (unless you use a multithreaded program.. No solution for this case now...)
    void sendSSE(const std::string& event, const json& data, std::string id="", int retry=-1);
    void sendSSE(const std::string& event, const std::string& data, std::string id="", int retry=-1);
// this will headers alowing all conexion to be accepted, not just from the same domain, you could should specific domains replasing the arg domains
    void sendCORSHeaders(const std::string& domains = "*");
    void sendSSEHeaders();
    void sendJSONHeaders(long contentSize = -1, bool close=false);
    void sendHtmlHeaders(bool close=false);

    void addOnDeconnection(boost::function <void ()>f);

    //need to call this function periodicly to check if the client is still connected.
    //If not Apache will send to this program a SIGTERM signal
    //you can add functions to be executed on thos signal with the function network::addOnDeconnection
    void checkConnection();

    //infos in $_GET
    std::unordered_map<std::string, std::string> get(const std::string& getstring="");

    //f is the function that will be execute at each loop, its need to renturn a json wich is the data send by the SSE (its take no argument)
    //interval is in milisecond
    //quit is a function retunring a bool that when is true will breake the while stream loop
    //This function is sync and will block until the stream loop is broken with the quit function or if the client disconnect
    template <typename F, typename F2=int>
        void streamSSE(const std::string& event, F f, int interval=1000, F2 quit=0, bool includeHeaders=true)
        {
            if (includeHeaders) 
            {
                network::sendSSEHeaders();
            }
            
            while(true)
            {
                json data = f();
                network::sendSSE(event, data);
                int secs = interval/1000;
                if (secs>0)
                {
                    for (int i=0; i<secs; i++)
                    {
                        network::checkConnection();
                        th::msleep(1000);
                    }
                }
                else 
                    th::msleep(interval);
                if constexpr (!std::is_same<F2, int>::value)
                {
                    if (quit())
                        break;
                }
            }

        }

    enum Method
    {
        GET = 1,
        POST,
    };

    enum DataType
    {
        STRING = 1,
        JSON
    };

    std::string curl();
    std::string send(std::string url, const json& data, Method method=POST);
    std::string send(std::string url, const std::string& data, Method method=POST);

    //this is server side not client
    std::string http_formated(const std::string& res, const std::string& contentType = "text/html", bool cors = true, int serverCode = 200);

    std::string url_encode(const std::string &value);
    std::string client_request_as_http(const std::string& path, const std::string& data, Method method=GET, DataType dataType=STRING, std::map<std::string, std::string> headers = {});

    std::string sse_headers();
    std::string sse_formated(const std::string& event, const std::string& data, std::string id, int retry=-1);
    std::string sse_formated(const std::string& event, const json& data, std::string id, int retry=-1);
}
