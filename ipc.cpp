#include "./ipc.h"
#include <unordered_map>
#include "./stds.h"
#include <mutex>
#include "./Perfs.h"
#include "./vec.h"
#include <map>
#include "./thread.h"
#include "./mlprocess.h"

namespace ipc
{
    unsigned int _reqId = 0;
    unsigned int reqId() {return _reqId++;}
    std::map<std::string, std::string> _args;

    th::Safe<std::unordered_map<std::string, ProcessCmd>> _registers("ipc::registers");

    th::Safe<std::vector<std::function<void()>>> _additianalCallbacks("ipc::_additianalCallbacks");

    std::atomic_bool _timerEmitThreadRunning = false;

    std::atomic<int> _max_event_rate = 16;
    struct EventQueue
    {
        // you can't emit an event more often than this (in ms)
        // TODO should be savable in .config... (see storage.h)
        Perfs timer;
        json queue = json::array();
    };

    th::Safe<EventQueue> _events_queue("ipc::events_queue");

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

        std::lock_guard l(_registers);
        if (_registers.data().find(function) != _registers.data().end())
        {
            lg("found function to call : " << function);
            // this is actually where the function is called
            auto& pcmd = _registers.data()[function];

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
            json j;
            try
            {
                j = json::parse(line);
            }
            catch(const std::exception& e)
            {
                lg("cannot parse line for ipc response : " << line);
                lg(e.what());
            }

            if (j["id"] == id)
            {
                try
                {
                    cb(j);
                }
                catch(const std::exception& e)
                {
                    lg("Error in the callback on the ipc response : " << e.what());
                    lg("line : " << line);
                }
                p->onOutput()[idx] = 0;
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
        if (_events_queue.data().queue.size() == 0)
            return;
        std::cout << _events_queue.data().queue.dump() << std::endl;
        _events_queue.data().queue = json::array();
        _events_queue.data().timer.start();
    }

    void emit(const std::string& event, const json& data)
    {
        json _r = {};
        _r["type"] = "event";
        _r["event"] = event;
        _r["data"] = data;

        {
            std::lock_guard l(_events_queue);
            _events_queue.data().queue.push_back(_r);

            //more than max_rate passed, so we can send it like this
            if (_events_queue.data().timer.hasPassed(_max_event_rate))
            {
                emit_events_queue();
                return;
            }
        }

        //less than max_rate passed, so we queue it
        bool expected = false;
        if (_timerEmitThreadRunning.compare_exchange_strong(expected, true))
        {
            auto emitf = []
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(_max_event_rate));

                {
                    std::lock_guard l(_events_queue);
                    emit_events_queue();
                }
                _timerEmitThreadRunning = false;
            };
            std::thread(emitf).detach();
        }
    }

    ProcessCmd& reg(const std::string& function,
            const std::function<json(const json& args)>& todo,
            const std::vector<std::string>& mendatoryKeys,
            const std::vector<std::string>& optionalKeys)
    {
        std::lock_guard l(_registers);
        auto _todo = [todo](const json& args, int id){return todo(args);};
        _registers.data()[function] = {_todo, function, mendatoryKeys, optionalKeys};
        lg("registered function : " << function);
        return _registers.data()[function];
    }

    ProcessCmd& reg(const std::string& function)
    {
        std::lock_guard l(_registers);
        _registers.data()[function] = {0, function};
        lg("registered function : " << function);
        return _registers.data()[function];
    }

    json ProcessCmd::onResponse(const json& response, int id)
    {
        json res;
        if (ipc::errorIfNotExists(response, res, this->mendatoryKeys, this->name))
            return res;
        return this->cb(response, id);	
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

                std::lock_guard l(_registers);
                if (_registers.data().find(funcname) != _registers.data().end())
                {
                    lg("found function to call : " << funcname);
                    // this is actually where the function is called
                    auto& pcmd = _registers.data()[funcname];
                    json& args = received["args"];
                    int id = 0;
                    if (received.contains("id"))
                        id = received["id"];

                    auto async = [&pcmd, args, id]
                    {
                        json res = {};
                        if (pcmd.cb)
                            res = pcmd.onResponse(args, id);

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
                std::lock_guard l(_additianalCallbacks);
                cbs = _additianalCallbacks.data();
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
            std::lock_guard l(_additianalCallbacks);
            _additianalCallbacks.data().push_back(f);
        }
    }

    void signal()
    {
        eventfd_t val = 1;
        eventfd_write(stds::efd(), val);
    }

    void log(const std::string& funcid)
    {
        std::lock_guard l(_registers);
        _registers.data()[funcid].log();
    }

    void logAll()
    {
        std::lock_guard l(_registers);
        for (auto& r : _registers.data())
            r.second.log();
    }

    void ProcessCmd::log()
    {
        std::string msg = this->name + "\n";
        if (this->mendatoryKeys.size() > 0)
        {
            msg += "  Mendatory keys : \n";
            for (auto& k : this->mendatoryKeys)
                msg += "    " + k + ",\n";
            msg += "\n";
        }

        if (this->optionalKeys.size() > 0)
        {
            msg += "  Optional keys : \n";
            for (auto& k : this->optionalKeys)
                msg += "    " + k + ",\n";
        }
        msg += "---------\n";

        std::cout << msg << std::endl;
    }
}
