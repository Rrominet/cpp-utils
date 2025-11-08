#include "./ipc.h"
#include <unordered_map>
#include "./stds.h"
#include <mutex>
#include "./Perfs.h"
#include "./vec.h"
#include <map>
#include "./mlprocess.h"

namespace ipc
{
    unsigned int _reqId = 0;
    unsigned int reqId() {return _reqId++;}
    std::mutex _registersMtx;
    std::mutex _callbacksMtx;
    std::mutex _events_queueMtx;
    std::map<std::string, std::string> _args;

    std::unordered_map<std::string, ProcessCmd> _registers; 

    std::vector<std::function<void()>> _additianalCallbacks;
    std::mutex _additianalCallbacksMtx;

    // you can't emit an event more often than this (in ms)
    // TODO should be savable in .config... (see storage.h)
    int _max_event_rate = 16;
    Perfs _eventsTimer;

    json _events_queue = json::array();

    void init(int argc, char *argv[])
    {
        _args = args::nparse(argc, argv);
    }

    bool execFromArgs()
    {
        std::string function;
        if (_args.find("function") == _args.end())
        {
            lg(" ipc::execFromArgs : no function specified, so ignored.");
            return false;
        }
        else 
        {
            function = _args["function"];
        }

        json args = {};
        try {
            if (_args.find("args") != _args.end())
                args = json::parse(_args["args"]);
            else 
                lg("ipc::execFromArgs : no args specified, not an error, executed without args.");
        } catch (const std::exception& e) {
            {
                lg("Error in ipc::execFromArgs : after/during json parse of args " << e.what()); 
                return false;
            }
        }

        std::lock_guard<std::mutex> l(_registersMtx);
        if (_registers.find(function) != _registers.end())
        {
            lg("found function to call : " << function);
            // this is actually where the function is called
            auto& pcmd = _registers[function];

            json res = {};
            if (pcmd.cb)
                res = pcmd.cb(args, -1);

            //used for the caller process to know which request it is responding to
            res["id"] = -1;
            res["type"] = "response"; // could be stream...

            // send the response via stdout :
            std::cout << res.dump() << std::endl;
            return true;
        }
        else 
        {
            lg("cannot find function to call : " << function);
            return false;
        }

    }

    void addToResponse(Process* p, unsigned int id, const std::function<void(const json&)>& cb)
    {
        auto idx = p->onOutput().size();
        auto f = [id, cb, idx, p](const std::string& line)
        {
            try
            {
                json j = json::parse(line);
                if (j["id"] == id)
                {
                    cb(j);
                    p->onOutput()[idx] = 0;
                }
            }
            catch(const std::exception& e)
            {
                lg("cannot parse line for ipc response : " << line);
                lg(e.what());
            }
        };

        p->addOnOutput(f);
    }

    void send(Process* p, const json& data, const std::function<void(const json&)>& cb=0)
    {
        json to_send = data; 
        to_send["id"] = reqId();
        p->write(to_send.dump());
        if (cb)
            addToResponse(p, to_send["id"], cb);
    }

    void call(Process*p, const std::string &function, const json& args, const std::function<void(const json& response)>& cb)
    {
        json to_send;
        to_send["function"] = function;
        to_send["args"] = args;
        if (!cb)
            send(p, to_send);
        else 
            send(p, to_send, cb);
    }

    void subscribe(Process*p, const std::string& event, const std::function<void(const json& response)>& cb)
    {
        auto f = [event, cb](const std::string& line)
        {
            json j;
            try
            {
                j = json::parse(line);
            }
            catch(const std::exception& e)
            {
                lg("cannot parse line for ipc response : " << line);
                lg(e.what());
                return;
            }

            if (j.type() == json::value_t::object)
            {
                if (!j.contains("type") || !j.contains("event"))
                {
                    lg("IPC response does not contains event : " << line);
                    return;
                }

                if (j["type"] == "event" && j["event"] == event)
                    cb(j);
            }

            else if (j.is_array())
            {
                for (const auto& el : j)
                {
                    if (!el.contains("type") || !el.contains("event"))
                    {
                        lg("IPC response does not contains event : " << line);
                        continue;
                    }

                    if (el["type"] == "event" && el["event"] == event)
                        cb(el);
                }
            }
        };

        p->addOnOutput(f);
    }

    void emit_events_queue()
    {
        {
            std::lock_guard<std::mutex> l(_events_queueMtx);
            if (_events_queue.size() == 0)
                return;
            std::cout << _events_queue.dump() << std::endl;
            _events_queue = json::array();
        }
        _eventsTimer.start();
    }

    void emit(const std::string& event, const json& data)
    {
        json _r = {};
        _r["type"] = "event";
        _r["event"] = event;
        _r["data"] = data;

        {
            std::lock_guard<std::mutex> l(_events_queueMtx);
            _events_queue.push_back(_r);
        }

        //more than max_rate passed, so we can send it like this
        if (_eventsTimer.hasPassed(_max_event_rate))
        {
            emit_events_queue();
            return;
        }

        //less than max_rate passed, so we queue it
        else
        {
            auto emitf = []
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(_max_event_rate));
                emit_events_queue();
            };
            std::thread(emitf).detach();
        }
    }

    ProcessCmd& reg(const std::string& function, const std::function<json(const json& args)>& todo)
    {
        std::lock_guard<std::mutex> l(_registersMtx);
        auto _todo = [todo](const json& args, int id){return todo(args);};
        _registers[function] = {_todo, function};
        lg("registered function : " << function);
        return _registers[function];
    }

    ProcessCmd& reg(const std::string& function)
    {
        std::lock_guard<std::mutex> l(_registersMtx);
        _registers[function] = {0, function};
        lg("registered function : " << function);
        return _registers[function];
    }

    std::function<void(const std::string&)> onStdInLine()
    {
        auto f = [](const std::string& line)
        {
            lg("received a new line in stdin for ipc, lets go...");
            if (!line.empty())
            {
                json received;
                try
                {
                    received = json::parse(line);
                }
                catch (const std::exception& e)
                {
                    lg("can't parse the request in json for ipc : " << e.what());
                    return;
                }

                std::string funcname;

                try
                {
                    funcname = received.at("function");
                }
                catch (const std::exception& e)
                {
                    lg("no key 'function' in the request for ipc : " << e.what());
                    return;
                }

                std::lock_guard<std::mutex> l(_registersMtx);
                if (_registers.find(funcname) != _registers.end())
                {
                    lg("found function to call : " << funcname);
                    // this is actually where the function is called
                    auto& pcmd = _registers[funcname];
                    json& args = received["args"];
                    int id = 0;
                    if (received.contains("id"))
                        id = received["id"];

                    auto async = [&pcmd, args, id]
                    {
                        json res = {};
                        if (pcmd.cb)
                            res = pcmd.cb(args, id);

                        //used for the caller process to know which request it is responding to
                        res["id"] = id;
                        res["type"] = "response"; // could be stream...

                        // send the response via stdout :
                        std::cout << res.dump() << std::endl;
                    };

                    std::thread(async).detach();
                }

                else 
                {
                    int id = received["id"];
                    json res = {};
                    res["id"] = id;
                    res["type"] = "response";
                    res["message"] = "function not found : " + funcname;
                    res["success"] = false;
                    std::cout << res.dump() << std::endl;
                }
            }

            std::vector<std::function<void()>> cbs;
            {
                std::lock_guard<std::mutex> l(_additianalCallbacksMtx);
                cbs = _additianalCallbacks;
            }
            
            for (const auto& cb : cbs)
                cb();
        };
        return f;
    }

    void stream(json& data, int id)
    {
        data["id"] = id;
        data["type"] = "stream";
        std::cout << data.dump() << std::endl;
    }

    void initAsReceiver()
    {
        stds::read_in_async(onStdInLine());
        lg("initAsReceiver done.");
    }

    void receive()
    {
        stds::init();
        stds::read_in(onStdInLine());
        lg("receive done.");
    }

    void error(json& toreturn, const std::string& message)
    {
        toreturn["success"] = false;
        toreturn["error"] = message;
    }

    // will set the json in a nerror mode if the key doesn't exist
    bool errorIfNotExists(const json& tocheck, json& toreturn, const std::string& key, const std::string& ctxmessage)
    {
        if (!tocheck.contains(key))
        {
            toreturn["success"] = false;
            toreturn["error"] = "Missing key : " + key + (ctxmessage.empty() ? "" : " (" + ctxmessage + ")");
            return true;
        }
        return false;
    }
    
    // will set the json in a nerror mode if the key doesn't exist
    bool errorIfNotExists(const json& tocheck, json& toreturn, const std::vector<std::string>& keys, const std::string& ctxmessage)
    {
        for (const auto& key : keys)
        {
            if (errorIfNotExists(tocheck, toreturn, key, ctxmessage))
                return true;
        }
        return false;
    }

    void success(json& toreturn)
    {
        toreturn["success"] = true;
        toreturn["error"] = "";
    }

    void setMaxEventRate(int ms)
    {
        _max_event_rate = ms;
    }

    int maxEventRate()
    {
        return _max_event_rate;
    }

    // the function will be executed after each read loop (in the data received from the process)
    // you can also run the read loop manually with signal()
    void addOnReadLoop(const std::function<void()>& f)
    {
        {
            std::lock_guard<std::mutex> l(_additianalCallbacksMtx);
            _additianalCallbacks.push_back(f);
        }
    }

    void signal()
    {
        eventfd_t val = 1;
        eventfd_write(stds::efd(), val);
    }
}
