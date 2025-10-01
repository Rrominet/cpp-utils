#pragma once

/*
 * Documentation : 
 * the scipts API is a back-end that allows you to execute scripts in any language in your application.
 *
 * How does this work : 
 * 1st : you need to add a script from a file on your computer (you can install it if you want (it will copy the file in the script folder - folder given in the init function that should be executed when your program start -- see the mention an the end of this doc for more infos on scripts::init.))
 * script_id = scripts::create("path/to/your/script/file", "path/to/the/engine", async or not)
 * you can choose to install your script like this : scripts::get(script_id).install() (but if the script is local to your computer and you don't want it to be copied in the scripts folder, you don't need to do it)
 *
 * 2nd : you need to add Events in you app to tell when your scripts should be executed.
 * for example : 
 * scripts::linkToEvent(script_id, YOUR_EVENT); // here YOUR_EVENT is just a unsigned int. You can use an enum if you want for convenience.
 *
 * 3rd : you need to "call" your event in your app to actually run your scripts linked with it (linked with scripts::linkToEvent)
 * to do so, just do : 
 * scripts::runEvent(YOUR_EVENT, funcOnScriptResponse, funcOnError); // becareful here, certain scripts could run async so DO NOT assume that the scripts will be finished running after this function.
 *
 * The basics are doned.
 *
 * BUT the API also provide you a way to save and load your configuration to have permanant scripts loaded when your program start.
 *
 * To do so, you need to call first : 
 * scripts::init("path/to/scripts/dir"); // you could call the async version to.
 *
 * the first time your run init, nothing will be doned because no scripts has been saved by the API.
 * But now you can save your scripts with : 
 * Script.save() 
 * or 
 * Script.install()  if you want to copy the script file in the scripts folder (see above)
 *
 * Once this is doned, the scripts will be loaded on your program start next time when calling scripts::init.
 *
 * Be careful here, you need to call scripts::linkToEvent BEFORE saving your script, or else your script will be save but with no event assocciated with it and so will never be executed the next time.
 *
 *
 * Communicating with your scripts :
 * to communicate to your script, you will need to use the function : 
 * scripts::parser::addFunction("my_function_name", func);
 * func sould return a string that will be replaced in the script (here my_function_name will be replaced with the return value of func, becareful of the "" for a string you should return "\"your string\"", or use the helper function scripts::parser::quoted("your string")).
 *
 * You can also react in your app from you script response (stdout)
 * for this you need to create a new command : 
 * scripts::parser::addCmd("my_cmd_name", func);
 * the func here take a json as arg (that represent the args passed to your command)
 * to work your script should write to stdout a json with at least {"cmd" : "my_cmd_name"}
 * be careful, if you print for debug, you will have errors because the json will not be parsed correctly.
 * if you want to debug your script, print to stderr, it will be loged in the console of your c++ app.
 * 
 * here is a working example for creating new scripts :
 *    scripts::parser::addFunction("fxplorer.hw()", []{return "\"Hello World !\"";});
    scripts::parser::addFunction("fxplorer.magic_number()", []{return "42";});

    scripts::parser::addCmd("create-file", [](const json& args){
            auto filepath = args["filepath"].get<std::string>();
            ml::File f(filepath);
            f.write(args["content"].get<std::string>());
            });

    enum {
        JS_EVENT, 
        PHP_EVENT, 
        PY_EVENT
    };

    auto js = scripts::create(path::execDir() + "/scripts/test-script-runner.js", "/usr/bin/node");
    auto php = scripts::create(path::execDir() + "/scripts/test-script-runner.php", "/usr/bin/php");
    auto py = scripts::create(path::execDir() + "/scripts/test-script-runner.py", "/usr/bin/python3");

    scripts::linkToEvent(js, JS_EVENT);
    scripts::linkToEvent(php, PHP_EVENT);
    scripts::linkToEvent(py, PY_EVENT);

    scripts::save(path::execDir() + "/scripts"); // save called AFTER linkToEvent

    auto onRes = [](const std::string& res)
    {
        lg(res);
    };

    auto onErr = [](const std::string& res)
    {
        lg("Error :");
        lg(res);
    };

    scripts::runEvent(PHP_EVENT, onRes, onErr);
    scripts::runEvent(PY_EVENT, onRes, onErr);
    scripts::runEvent(JS_EVENT, onRes, onErr);
 *
 *
 * here is a working example that read the previously created scripts : 
 *
 *  scripts::parser::addFunction("fxplorer.hw()", []{return "\"Hello World !\"";});
    scripts::parser::addFunction("fxplorer.magic_number()", []{return "42";});

    // we create a cmd that write a simple file with the arguments provided by the script.
    scripts::parser::addCmd("create-file", [](const json& args){
            auto filepath = args["filepath"].get<std::string>();
            ml::File f(filepath);
            f.write(args["content"].get<std::string>());
            });

    enum {
        JS_EVENT, 
        PHP_EVENT, 
        PY_EVENT
    };

    scripts::init(path::execDir() + "/scripts");
    auto onRes = [](const std::string& res)
    {
        lg(res);
    };

    auto onErr = [](const std::string& res)
    {
        lg("Error :");
        lg(res);
    };

    scripts::runEvent(PHP_EVENT, onRes, onErr);
    scripts::runEvent(PY_EVENT, onRes, onErr);
    scripts::runEvent(JS_EVENT, onRes, onErr);
 */

#include "str.h"

namespace scripts
{
    typedef unsigned int Event;
    class Script
    {
        private:
            std::string _content;

        public : 

            std::string filepath;
            bool async = false;
            bool reload = false;
            std::string engine = "";
            ml::Vec<Event> linkedEvents;
            
            // Default constructor
            Script() = default;

            // Copy constructor
            Script(const Script& other);

            // Copy assignment operator
            Script& operator=(const Script& other);

            std::string content();
            void run(const std::function<void(const std::string&)>& func = 0, const std::function<void(const std::string&)>& onError = 0);
            std::string findEngineFlag() const ;

            std::string name() const;
            void save(const std::string& scriptsdir) const;
            void install(const std::string& scriptsdir);

            // the filepath of the script should be set
            void read(const std::string& scriptsdir, unsigned int idx);
    };

    namespace parser
    {
        //You can pass information to your scripts through the addFunction (and other functions) API.
        //
        //You react to your script after its execution, through the addCmd API.
        //The information send by your scripts to this program should be send via stdout in JSON format.


        // the function in _api should return a string that will be directly replaced in the script.
        // for example 
        // the function : 
        // this key "myapi.int()" will replace all instance of "myapi.function()" in the script with the result the retunred value from its value
        // for example [=]{return "3";}
        //
        // myapi.string() sould have this form :  (or should be put inside " " in the script)
        // [=]{return "\"my string\"";}
        //
        void addFunction(const std::string &key, const std::function<std::string()>& func);
        void removeFunction(const std::string &key);
        void clearFunctions();

        std::string quoted(const std::string& str);


        // a cmd work like a function BUT its need to be send by the script itself through stdout in a json like : 
        // {"cmd", : "my_cmd_name"}
        // you can put several other things in the json object
        // there will be passed in the json argument in the 2nd argument here.
        // see you json content like the args passed to you cmd.
        // be careful, if your scripts is async, the function liked to key will be called on a different thread, so you should always have a code that is threadsafe in func !
        void addCmd(const std::string &key, const std::function<void(const json&)>& func);
        void removeCmd(const std::string &key);
        void clearCmds();
        void execCmdsOnStdout(const std::string& std_out);

        std::string constentParsed(Script& script);
    }

    std::string run(const std::string& filepath, const std::string& engine);
    void run_async(const std::string& filepath, const std::string& engine, const std::function<void(const std::string&)>& func = 0, const std::function<void(const std::string&)>& onError = 0);

    // the return value is the index to access the script later via scripts::get(i)
    unsigned int create(const std::string& filepath, const std::string& engine, bool async = false);

    // check the filename of the script
    unsigned int scriptFromName(const std::string& name);
    unsigned int scriptFromFilepath(const std::string& filepath);
    std::vector<Script>& _();

    Script& get(unsigned int idx);
    void remove(unsigned int idx);

    void linkToEvent(unsigned int scriptIdx, Event event);

    // will run all scripts linked to the event
    // careful here, some scripts could be async so do not assume that the scripts will be finished running after this function.
    void runEvent(Event event, const std::function<void(const std::string&)>& func = 0, const std::function<void(const std::string&)>& onError = 0);

    
    // you should call this function to init the scripts folder
    // and load the scripts in it.
    void init(const std::string& dirpath);
    void init_async(const std::string& dirpath, const std::function<void()>& onDoned=0, const std::function<void(const std::string&)>& onError = 0);

    //it will create all the scripts .conf files in the dir to reloasd them later.
    //if install is true, the scripts will be installed in the scripts folder (the files will be copied)
    void save(const std::string& dirpath, bool install=false);
    void save_async(const std::string& dirpath, bool install=false, const std::function<void()>& onDoned=0, const std::function<void(const std::string&)>& onError = 0);
}
