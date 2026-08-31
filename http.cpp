#include "http.h"
#include "str.h"
#include "files.2/files.h"
#include "os.h"

#include <iomanip>

namespace http
{
    std::vector<Request*> _requests;
    std::string curl = "curl";
    bool _find_curl_called = false;
    bool _curl_overriden = false;

    std::vector<std::string> _possible_curl_paths;
    bool _possible_curl_paths_initialized = false;

    void _init_possible_curl_paths()
    {
        if (_possible_curl_paths_initialized)
            return;

        _possible_curl_paths_initialized = true;

        if (os::type() == "linux" || os::type() == "ios")
        {
            _possible_curl_paths.push_back("/usr/bin/curl");
            _possible_curl_paths.push_back("/usr/share/bin/curl");
            _possible_curl_paths.push_back("/usr/local/bin/curl");
            _possible_curl_paths.push_back("/usr/local/curl");
            _possible_curl_paths.push_back("/usr/local/bin/curl");
            _possible_curl_paths.push_back("/usr/local/curl/bin/curl");
        }

        else if (os::type() == "windows")
        {
            _possible_curl_paths.push_back("C:\\Program Files\\curl\\bin\\curl.exe");
            _possible_curl_paths.push_back("C:\\Program Files\\curl\\curl.exe");
            _possible_curl_paths.push_back("C:\\Program Files\\curl.exe");
        }
    }

    bool _program_exists(std::string program)
    {
        return files::exists(program) && !files::isDir(program);
    }

    void _find_curl()
    {
        if (_curl_overriden)
            return;
        if (_find_curl_called)
            return;

        _init_possible_curl_paths();

        for (const auto& p : _possible_curl_paths)
        {
            if (_program_exists(p))
            {
                curl = p;
                return;
            }
        }
        _find_curl_called = true;
    }

    void overrideCurlPath(std::string path)
    {
        _curl_overriden = true;
        curl = path;
    }

    void destroyTermintatedRequests()
    {
        for (int i=0; i<_requests.size(); i++)
        {
            if (_requests[i] && !_requests[i]->running())
            {
                delete _requests[i];
                _requests[i] = nullptr;
            }
        }
    }

    void addRequest(Request* process)
    {
        for (int i=0; i<_requests.size(); i++)
        {
            if (!_requests[i])
            {
                _requests[i] = process;
                return;
            }
        }

        _requests.push_back(process);
    }

    std::string urlEncoded(const std::map<std::string, std::string> &params)
    {
        std::string _r = "";
        for (const auto &[k, v] : params)
            _r += urlEncoded(k) + "=" + urlEncoded(v) + "&";

        _r.pop_back();
        return _r;
    }

    std::string urlEncoded(std::string data)
    {
        data = str::replace(data, "&", "%26");
        data = str::replace(data, "\"", "'");
        return data;
    }

    std::string cmd(std::string url, 
            const std::string& sdata, 
            const std::map<std::string, std::string> &params,
            json data, 
            Method method, ml::Vec<std::string> headers)
    {
        auto cmd = curl ;
        cmd += " -sS";
        for (const auto& h : headers)
            cmd += " -H \"" + h + "\"";
        if (method == POST)
        {
            cmd += " -X POST";
            if (!sdata.empty())
                cmd += " -d " + str::quote(sdata); 
            else if (params.size() != 0)
                cmd += " -d " + str::quote(urlEncoded(params));
            else if (data.size() != 0)
                cmd += " -H 'Content-type: application/json' -d '" + str::replace(data.dump(), "'", "'\\''") + "'";
        }
        else 
        {
            if (params.size() != 0)
                url += "?" + urlEncoded(params);
        }

        cmd += " " + str::quote(url);
        lg(cmd);
        return cmd;
    }

    ml::Vec<std::string> cmd_as_vector(std::string url, 
            const std::string& sdata, 
            const std::map<std::string, std::string> &params,
            json data, 
            Method method,
            ml::Vec<std::string> headers)
    {
        ml::Vec<std::string> result;
        result.push(curl);
        result.push("-sS");

        for (const auto& h : headers)
        {
            result.push("-H");
            result.push(h);
        }

        if (method == POST)
        {
            result.push("-X");
            result.push("POST");

            if (!sdata.empty())
            {
                result.push("-d");
                result.push(sdata);
            }
            else if (params.size() != 0)
            {
                result.push("-d");
                result.push(urlEncoded(params));
            }
            else if (data.size() != 0)
            {
                result.push("-H");
                result.push("Content-type: application/json");
                result.push("-d");
                result.push(data.dump());
            }
        }
        else
        {
            if (params.size() != 0)
                url += "?" + urlEncoded(params);
        }

        result.push(url);
        return result;
    }

    ProcessOut request_sync(std::string url, 
            const std::string& sdata, 
            const std::map<std::string, std::string> &params,
            json data, 
            Method method)
    {
        _find_curl();
        auto command = cmd_as_vector(url, sdata, params, data, method);
        return process::exec2(command.vec);
    }

    ProcessOut get_sync(std::string url,
            const std::map<std::string, std::string> &params)
    {
        return request_sync(url, "", params, {}, GET);
    }

    ProcessOut post_sync(std::string url, json data, 
            const std::map<std::string, std::string> &params)
    {
        return request_sync(url, "", params, data, POST);
    }

    void request(std::string url,
            const std::string& sdata,
            const std::map<std::string, std::string> &params, 
            json data,
            Method method,
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr, 
            boost::function<void (std::string)> onError)
    {
        _find_curl();
        destroyTermintatedRequests();

        auto c = cmd(url, sdata, params, data, method);
        auto r = new Request(c);
        addRequest(r);

        if (onDoned)
        {
            auto f = [=]{
                onDoned(r->output());
            };
            r->addOnEnd(f);
        }
        if (onPgr)
        {
            auto f = [=](const std::string& line){
                onPgr(r->output());
            };
            r->addOnOutput(f);
        }
        if (onError)
        {
            auto f = [=](const std::string& line){
                onError(r->output());
            };
            r->addOnError(f);
        }

        r->start();
    }

    void get(const std::string& url,
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr, 
            boost::function<void (std::string)> onError)
    {
        request(url, "", {}, {}, GET, onDoned, onPgr, onError);
    }

    void get(const std::string& url, std::map<std::string, std::string> params, 
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr, 
            boost::function<void (std::string)> onError)
    {
        request(url, "", params, {}, GET, onDoned, onPgr, onError);
    }

    void post(const std::string& url, std::map<std::string, std::string> params, 
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr, 
            boost::function<void (std::string)> onError)
    {
        request(url, "", params, {}, POST, onDoned, onPgr, onError);
    }

    void post(const std::string& url, json data, 
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr, 
            boost::function<void (std::string)> onError)
    {
        request(url, "", {}, data, POST, onDoned, onPgr, onError);
    }

    void post(const std::string& url, const std::string& data, 
            boost::function<void (std::string)> onDoned, 
            boost::function<void (std::string)> onPgr, 
            boost::function<void (std::string)> onError)
    {
        request(url, data, {}, {}, POST, onDoned, onPgr, onError);
    }

    void test()
    {
        json data = {} ;
        data["conv-id"] = "romain.gilliot_a_motion-live.com";
        data["cmd"] = "new-message";
        data["content"] = "This is a test msg";
        data["sender"] = {};
        data["sender"]["name"] = "Romain (formateur)";
        data["sender"]["ip"] = "host";
        data["tchat-dir"] = "site-teach/formations/01-h3d2/data/tchats";

        http::post("https://motion-live.com/frameworks/bin/tchat.cgi", data, [=](auto res)
                {
                    if (!res.empty())
                        std::cout << "Post request response is NOT empty." << std::endl;
                    else 
                        std::cout << "Error : http response is EMPTY." << std::endl;

                    try 
                    {
                        auto d = json::parse(res);
                        if (d.contains("success") && d["success"])
                            std::cout << "HTTP request succeeded. :)" << std::endl;
                        else 
                        {
                            if (!d.contains("success"))
                                std::cout << "Response don't contains success in json object" << std::endl;
                            else if (d["success"] == false)
                                std::cout << "Response contain 'success' but its equal to false." << std::endl;
                        }
                    }
                    catch(...)
                    {
                        std::cout << "Error : can't parse the json response" << std::endl;
                    }
                    std::cout << "Response : " << res << std::endl;
                });

        std::map<std::string, std::string> mapd;
        mapd["func"] = "sendEmail";
        mapd["email"] = "romain.gilliot@motion-live.com";
        mapd["object"] = "Test from cpp";
        mapd["content"] = "\"<b>Test from cpp code</b>\".";
        
        http::post("https://teach.motion-live.com/ajaxCompte.php", mapd, [=](auto res){
                if (!res.empty())
                    std::cout << "Post request response is NOT empty." << std::endl;
                else 
                    std::cout << "Error : http response is EMPTY." << std::endl;
                if (res == "true\n")
                    std::cout << "http request with map works." << std::endl;
                else 
                    std::cout << "http response dont seams to be right" << std::endl;
                std::cout << "Response : " << res << std::endl;
                });
    }

    std::string get(const std::string& url, ml::Vec<std::string> headers)
    {
        auto c = cmd(url, "", {}, {}, GET, headers);
        return process::exec(c);
    }
    std::string get(const std::string& url, const std::map<std::string, std::string> &params, ml::Vec<std::string> headers)
    {
        auto c = cmd(url, "", params, {}, GET, headers);
        return process::exec(c);
    }

    std::string post(const std::string& url, std::map<std::string, std::string> params, ml::Vec<std::string> headers)
    {
        auto c = cmd(url, "", params, {}, POST, headers);
        return process::exec(c);
    }
    std::string post(const std::string& url, json data, ml::Vec<std::string> headers)
    {
        auto c = cmd(url, "", {}, data, POST, headers);
        return process::exec(c);
    }
    std::string post(const std::string& url, const std::string& data, ml::Vec<std::string> headers)
    {
        auto c = cmd(url, data, {}, {}, POST, headers);
        return process::exec(c);
    }
}
