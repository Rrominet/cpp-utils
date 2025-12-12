#pragma once
#include <boost/process.hpp>
#include <functions.h>
#include "./vec.h"
#include "./thread.h"

#ifdef _WIN32
#include <boost/process/windows.hpp>
#endif

namespace bp = boost::process;

struct ProcessRuntimeData
{
    bool running;
    std::unique_ptr<bp::child> process;
    std::unique_ptr<std::thread> runthread;
    std::unique_ptr<std::thread> outthread;
    std::unique_ptr<std::thread> errthread;
};

class Process
{
    public : 
        Process();
        Process(const std::string& cmd);
        Process(const std::string& cmd, const std::string& cwd);
        Process(const std::vector<std::string>& cmd);
        Process(const std::vector<std::string>& cmd, const std::string& cwd);
        ~Process();

        std::string cmd_s() const;
        std::vector<std::string> cmd() const {return _cmd;}
        void setCmd_s(const std::string &cmd);
        void setCmd(const std::vector<std::string>& cmd){_cmd = cmd;}

        std::string cwd() const {return _cwd;}
        void setCwd(const std::string &cwd){_cwd = cwd;}

        void addOnStart(std::function<void()> f);

        unsigned int addOnOutput(const std::function<void(const std::string& line)> &f);
        unsigned int addOnOutputBin(const std::function<void(const std::vector<unsigned char>& data_chunk)> &f);
        void addOnError(const std::function<void(const std::string& line)> &f);
        void addOnEnd(const std::function<void()> &f);
        void addOnTerminate(const std::function<void()> &f);
        void addOnProcessError(const std::function<void()> &f);

        void start();
        void terminate(bool sigkill=false);
        bool running();

        void wrapInScript();

        std::string output();
        std::string error();
        std::string processError();
        void write(const std::string& string);

        ml::Vec<std::function<void(const std::string& line)>>& onOutput(){return _onOutput;}

        void treatOutputAsBinary(bool val=true){_treatOutputAsBinary = val;}
        void setOutBinaryBufSize(size_t size){_outBinaryBufSize = size;}
        size_t outBinaryBufSize()const {return _outBinaryBufSize;}

    private : 
        // Configuration
        std::vector<std::string> _cmd;
        std::string _cwd;
        
        // Output storage
        std::string _processError;

        th::Safe<ProcessRuntimeData> _syncRuntime;
        th::Safe<std::string> _output;
        th::Safe<std::string> _error;

        // Callbacks
        ml::Vec<std::function<void()>> _onStart;
        ml::Vec<std::function<void(const std::string& line)>> _onOutput;
        ml::Vec<std::function<void(const std::vector<unsigned char>& data_chunk)>> _onOutputBin;
        ml::Vec<std::function<void(const std::string& line)>> _onError;
        ml::Vec<std::function<void()>> _onEnd;
        ml::Vec<std::function<void()>> _onTerminate;
        ml::Vec<std::function<void()>> _onProcessError;

        // Streams
        bp::ipstream _stream_output;
        bp::ipstream _stream_error;

        th::Safe<bp::opstream> _stream_input;

        // Binary output handling
        bool _treatOutputAsBinary;
        size_t _outBinaryBufSize;

        // Internal methods
        void _init();
        void _processOutputStreamAsBinary();
        void _cleanup();
        void _joinThreads();

        th::ThreadChecker _checker;
};


namespace args
{
    // its better to use this, it's more os standard. (use - and -- args not = args like the function above)
    std::map<std::string, std::string> nparse(const int& argc, char* argv[]);

    //alias for nparse
    std::map<std::string, std::string> parse(const int& argc, char* argv[]);

    // return juste a vector with the arguments, take into account delimiter space, ", '
    std::vector<std::string> simpleParse(const std::string& cmd);
}

namespace process
{
    //will take stdout or stderr and exec cb for every line (\r or \n)
    void getProcessStream(bp::ipstream& stream, const ml::Vec<std::function<void(const std::string& line)>>& cbs, th::Safe<std::string>* out=nullptr);
    std::vector<std::string> parse(const std::string& cmd);
    std::string to_string(const std::vector<std::string>& cmd);

    std::string exec(const char* cmd, const char* workingdir);
    std::string exec(const std::string& cmd, const std::string& workingdir="");
    std::string exec(const std::vector<std::string>& cmd, const std::string& workingdir="");

    // this use boot, not the os specific pipes
    // if working dir is empty, the current working dir is used
    // the output from stderr will be stored in error if not null
    std::string exec(const std::string& cmd, std::string inData, const std::string& workingdir, const bp::environment& env = boost::this_process::environment(), std::string* error=nullptr);

    Process* start(const std::string& cmd, const std::string& workingdir="");

    Process* start(const std::string& cmd, std::function<void ()> onDone, const std::string& workingdir="");
    //start allias
    Process* launch(const std::string& cmd, const std::string& workingdir="");
    Process* launch(const std::string& cmd, std::function<void ()> onDoned, const std::string& workingdir="");
    std::string bashparsed(std::string data);

    void chain(const std::vector<Process*>& processes, std::function<void ()> onDoned=0);
#ifdef __linux__
    pid_t currentId();
#elif _WIN32
    DWORD currentId();
#endif
    void end(int pgrId);
    void kill(int pgrId);

    int system(const std::string& cmd);

    // the difference with start/launch is that here the spawned process is completely independant
    // Due to some gtk limits, you can't use this function to launch a process from a gtk GUI. Use glib.g_spwan_* instead.
    void spawn(const std::string& cmd, const std::string& logFile="");

    //remove all ainsi color codes and others stuffs that could be in a stdout for a terminal
    std::string onlyReadables(const std::string& input);
    std::vector<std::string> wrappedInScript(const std::vector<std::string>& cmd);
}
