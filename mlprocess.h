#pragma once
#include <boost/process.hpp>
#include <functions.h>
#include "./vec.h"
#include "./NamedMutex.h"

#ifdef _WIN32
#include <boost/process/windows.hpp>
#endif

namespace bp = boost::process;

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

        // the return value is the index of the function in the list
        unsigned int addOnOutput(const std::function<void(const std::string& line)> &f);
        unsigned int addOnOutputBin(const std::function<void(const std::vector<unsigned char>& data_chunk)> &f);
        void addOnError(const std::function<void(const std::string& line)> &f);
        void addOnEnd(const std::function<void()> &f);
        void addOnTerminate(const std::function<void()> &f);
        void addOnProcessError(const std::function<void()> &f);

        void start();
        void terminate(bool sigkill=false);
        bool running(){return _running;}

        // this will wrap the command in a script like this : 
        // script -q -c "your cmd" /dev/null
        // so that the output will be interpretated as if in a TTY and so will be flushed when seeing the \r character (useful to track progress of yt-dlp, unrar, or whatever)
        void wrapInScript();

        std::string output();

        //return teh output of std::err from the child process
        std::string error();

        //return the string returned by boost if there was an error durrint launching the process
        std::string processError();
        void write(const std::string& string);

        ml::Vec<std::function<void(const std::string& line)>>& onOutput(){return _onOutput;}

        void treatOutputAsBinary(bool val=true){_treatOutputAsBinary = val;}
        void setOutBinaryBufSize(size_t size){_outBinaryBufSize = size;}
        size_t outBinaryBufSize()const {return _outBinaryBufSize;}

    private : 
        std::vector<std::string> _cmd;
        std::string _cwd;
        std::string _output;
        std::string _error;
        std::string _processError;
        ml::NamedMutex _runmtx;
        ml::NamedMutex _mtx;
        ml::NamedMutex _emtx;

        std::atomic<bool> _running = false;
        std::atomic<bool> _can_start = true;
        std::unique_ptr<bp::child> _process = nullptr;

        ml::Vec<std::function<void()>> _onStart;
        ml::Vec<std::function<void(const std::string& line)>> _onOutput;
        ml::Vec<std::function<void(const std::vector<unsigned char>& data_chunk)>> _onOutputBin;
        ml::Vec<std::function<void(const std::string& line)>> _onError;
        ml::Vec<std::function<void()>> _onEnd;
        ml::Vec<std::function<void()>> _onTerminate;

        ml::Vec<std::function<void()>> _onProcessError;

        bp::ipstream _stream_output;
        bp::ipstream _stream_error;
        bp::opstream _stream_input;

        // called in every constructors
        void _init();

        bool _treatOutputAsBinary = false;
        size_t _outBinaryBufSize = 8192;

        void _processOutputStreamAsBinary();

        ml::NamedMutex _runthmtx;
        std::unique_ptr<std::thread> _runthread = nullptr;
        std::unique_ptr<std::thread> _outthread = nullptr;
        std::unique_ptr<std::thread> _errthread = nullptr;
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
    void getProcessStream(bp::ipstream& stream, const ml::Vec<std::function<void(const std::string& line)>>& cbs, std::string* out=nullptr, ml::NamedMutex* mtx=nullptr);
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
