#pragma once

#include <thread>
#include "mlprocess.h"
#include <functional>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// a simple way to send and receive functions between processes

//all of the function in this file if take a Process* as argument, it should be running.
//all of the callbacks are executed on a different thread, so do not execute GUI stuffs in it.
namespace ipc 
{
    struct ProcessCmd
    {
        // the id is mandatory because you need to know which response is for which request (specially when async calls are used)
        std::function<json(const json&, int id)> cb = 0;
        std::string name;
        std::vector<std::string> mendatoryKeys;
        std::vector<std::string> optionalKeys;

        //abstract the cb call and the mendatory keys check.
        json onResponse(const json& response, int id);
        void log();
    };

    //mendatory if you want to exec the functions registered from arguments.
    //the arguments are like this : 
    //your-program -function funcname -args "{...}"
    void init(int argc, char *argv[]);

    //return true if executed else false.
    //return false if there is nothing to excecute for example.
    bool execFromArgs();

    //if sync is true, the function will be executed syncronolsy in the backend process, other call will need to wait for it to be executed
    void call(Process*p, const std::string &function, const json& args, const std::function<void(const json& response)>& cb=0, bool sync=false);
    //reg for register fucntion
    //the json returned by the function is the response from the function that the process caller will receive.
    //the json returned json could follow this standard : 
    //{
    //    "success" : true/false,
    //    "message" : "some message to add context or infos if the call was successfull",
    //    "data" : {...},
    //    "error" : "some error message if the call failed"
    //}

    ProcessCmd& reg(const std::string& function,
            const std::function<json(const json& args)>& todo,
            const std::vector<std::string>& mendatoryKeys={},
            const std::vector<std::string>& optionalKeys={});

    //useful if you need to reference to the ProcessCmd in your callback (for all ProcessCmd.stream(..) for example)
    ProcessCmd& reg(const std::string& function);

    // means that this process (the one running the code) is setted to register function that other processes can call with send (or higher level functions)
    // if this is don't called, the process won't be able to receive the send requests.
    void initAsReceiver();

    // this is like initAsReceiver but its blocking...
    // it's useful when your program should only react to a registered functions.
    void receive();

    // utility function to return an error in a returned json of a registered fucntion
    void error(json& toreturn, const std::string& message);

    // will set the json in a error mode if the key doesn't exist
    // return true in case of error
    bool errorIfNotExists(const json& tocheck, json& toreturn, const std::string& key, const std::string& ctxmessage="");

    // will set the json in a error mode if on of the keys doesn't exist
    // return true in case of error
    bool errorIfNotExists(const json& tocheck, json& toreturn, const std::vector<std::string>& keys, const std::string& ctxmessage="");

    // return false if the value is not the same
    template <typename T>
    bool checkValue(json& tocheck, json& toreturn, const std::string& key, const T& value)
    {
        T readed;
        try
        {
            readed = toreturn[key].get<T>(); 
        }
        catch(const std::exception& e)
        {
            toreturn["error"] = "can't read key " + key + " : " + e.what();
            toreturn["success"] = false;
            return false;
        }

        if (readed != value)
        {
            toreturn["error"] = "wrong value for key " + key;
            toreturn["success"] = false;
            return false;
        }

        return true;
    }

    void success(json& toreturn);

    void stream(json& data, int id);

    void subscribe(Process*p, const std::string& event, const std::function<void(const json& response)>& cb);


    // this will automaticly queue the event emission to stdout if the event rate is smaller than the max rate. (default is 16ms)
    // when queued, the queue will be send after the max rate time
    void emit(const std::string& event, const json& data={});

    // if not setted, it's 16ms
    void setMaxEventRate(int ms);
    int maxEventRate();

    // the function will be executed after each read loop (in the data received from the process)
    // you can also run the read loop manually with signal()
    void addOnReadLoop(const std::function<void()>& f);

    //just call this from a different thread from the main (the one you called receive on) to "wake up" the receive loop and execute any function that you added with addOnReadLoop
    void signal();

    //show function infos (like mendatory keys, optional keys, etc.)
    void log(const std::string& funcid);
    void logAll();
}
