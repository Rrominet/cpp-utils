#include "./mlprocess.h"
#include "str.h"
#include "os.h"
#include "exceptions.h"
#include <memory>
#include <mutex>
#include <stdexcept>
#include <signal.h>
#include <iostream>
#include <regex>
#include "files.2/files.h"
#include "thread.h"

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#elif _WIN32
#include <direct.h>
#include <windows.h>
#include <io.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#endif

#define PROPS _syncRuntime("Process Mutex"), _output("Output Mutex"), _error("Error Mutex"),\
       _treatOutputAsBinary(false), _outBinaryBufSize(8192), _checker()


Process::Process() : PROPS
{
    _init();
}

Process::Process(const std::string& cmd) 
    : PROPS
{
    _init();
    this->setCmd_s(cmd);
}

Process::Process(const std::string& cmd, const std::string& cwd) 
    : PROPS
{
    _init();
    this->setCmd_s(cmd);
    this->setCwd(cwd);
}

Process::Process(const std::vector<std::string>& cmd) 
    : PROPS
{
    _init();
    this->setCmd(cmd);
}

Process::Process(const std::vector<std::string>& cmd, const std::string& cwd) 
    : PROPS
{
    _init();
    this->setCmd(cmd);
    this->setCwd(cwd);
}

void Process::_init()
{
    _checker.check();
    auto reset_outputs = [this]
    {
        {
            th_guard(_output);
            _output.data().clear();
        }
        {
            th_guard(_error);
            _error.data().clear();
        }
        _processError = "";
    };

    this->addOnEnd(reset_outputs);
    this->addOnTerminate(reset_outputs);

//     this->addOnProcessError([this]{
//             auto f = [this]{
//             this->onError("Process Error : " + this->error() + "Debug informations :\n" + this->output());
//             };
//             ml::app()->queue(f);
//             });

#ifdef mydebug
    auto outdebug = [this](const std::string& s)
    {
        files::append(files::execDir() + files::sep() + files::name(vc::last(_cmd)) + "-pc_out", s + "\n");
    };

    auto errdebug = [this](const std::string& s)
    {
        files::append(files::execDir() + files::sep() + files::name(vc::last(_cmd)) + "-pc_err", s + "\n");
    };
    this->addOnOutput(outdebug);
    this->addOnError(errdebug);

    this->addOnTerminate([this]{
            files::write(files::execDir() + files::sep() + files::name(vc::last(_cmd)) + "-pc_terminate", "Process " + files::name(vc::last(_cmd)) + " terminated. This shoudn't hapenning.");
            });
#endif
}

Process::~Process()
{
    this->terminate(true);
    _joinThreads();
    _cleanup();
    lg("~Process");
}

void Process::_cleanup()
{
    _checker.check();
    {
        th_guard(_stream_input);
        if (_stream_input.data().pipe().is_open())
            _stream_input.data().pipe().close();
    }
    if (_stream_output.pipe().is_open())
        _stream_output.pipe().close();
    if (_stream_error.pipe().is_open())
        _stream_error.pipe().close();
}

void Process::_joinThreads()
{
    _checker.check();

    // Extract threads from the Safe wrapper
    std::unique_ptr<std::thread> runth;
    std::unique_ptr<std::thread> outth;
    std::unique_ptr<std::thread> errth;
    
    {
        std::lock_guard l(_syncRuntime);
        runth = std::move(_syncRuntime.data().runthread);
        outth = std::move(_syncRuntime.data().outthread);
        errth = std::move(_syncRuntime.data().errthread);
    }
    // Lock released here
    
    // Now join WITHOUT holding the lock
    if (runth && runth->joinable())
        runth->join();
    if (outth && outth->joinable())
        outth->join();
    if (errth && errth->joinable())
        errth->join();

}

void Process::addOnStart(std::function<void()> f)
{
    _checker.check();
    _onStart.push(f);
}

unsigned int Process::addOnOutput(const std::function<void(const std::string& line)> &f)
{
    _checker.check();
    _onOutput.push(f);
    return _onOutput.size() - 1;
}

unsigned int Process::addOnOutputBin(const std::function<void(const std::vector<unsigned char>& data_chunk)> &f)
{
    _checker.check();
    _onOutputBin.push(f);
    return _onOutputBin.size() - 1;
}

void Process::addOnError(const std::function<void(const std::string& line)> &f)
{
    _checker.check();
    _onError.push(f);
}

void Process::addOnEnd(const std::function<void()> &f)
{
    _checker.check();
    _onEnd.push(f);
}

void Process::addOnTerminate(const std::function<void()> &f)
{
    _checker.check();
    _onTerminate.push(f);
}

void Process::addOnProcessError(const std::function<void()> &f)
{
    _checker.check();
    _onProcessError.push(f);
}

void Process::setCmd_s(const std::string &cmd)
{
    _checker.check();
    lg("settings _cmd..");
    _cmd = process::parse(cmd);
    lg("_cmd setted : " + cmd);
}

std::string Process::cmd_s() const
{
    return process::to_string(_cmd);	
}

bool Process::running()
{
    std::lock_guard l(_syncRuntime);
    return _syncRuntime.data().running;
}

void Process::start()
{
    _checker.check();
    {
        std::lock_guard l(_syncRuntime);
        if (_syncRuntime.data().running)
        {
            lg("Process already running. Abort.");
            return;
        }

        if (_cmd.empty())
            throw error::missing_arg("cmd is empty ! No process to launch.");

        if (_cwd.empty())
            _cwd = os::home();

        if (!files::isDir(_cwd))
            throw error::dir_not_found("current working dir is not a dir !");

        // Join previous run thread if it exists
        if (_syncRuntime.data().runthread && _syncRuntime.data().runthread->joinable())
            _syncRuntime.data().runthread->join();

        _syncRuntime.data().running = true;
    }

    // Start the run thread outside the lock
    auto run_func = [this]() 
    {
        lg("starting process...");
        lg("Process " << this);
        lg("cmd : " + this->cmd_s());

        lg("a");

        {
            std::lock_guard l(_stream_input);
            _stream_input.data() = bp::opstream();
        }
        lg("a");
        _stream_output = bp::ipstream();
        lg("a");
        _stream_error = bp::ipstream();
        lg("a");

        try
        {
            {
        lg("a");
                std::lock_guard l(_syncRuntime);
                std::lock_guard l2(_stream_input);
        lg("a");
#ifdef _WIN32
                _syncRuntime.data().process = std::make_unique<bp::child>(_cmd, bp::start_dir(_cwd), bp::std_out > _stream_output, 
                                                       bp::std_err > _stream_error, bp::std_in < _stream_input.data(), 
                                                       bp::windows::create_no_window);
#else
        lg("a");
                _syncRuntime.data().process = std::make_unique<bp::child>(_cmd, bp::start_dir(_cwd), bp::std_out > _stream_output, 
                                                       bp::std_err > _stream_error, bp::std_in < _stream_input.data());
        lg("a");
#endif
            }

            lg2("process", this->cmd_s());
            lg2("current working dir", _cwd);
            lg("process {" + this->cmd_s() + "} just started in dir " + _cwd);

            // Start output handling threads
            if (!_treatOutputAsBinary)
            {
                std::lock_guard l(_syncRuntime);
                _syncRuntime.data().outthread = std::make_unique<std::thread>([this]{
                    process::getProcessStream(_stream_output, _onOutput, &_output);
                });
            }
            else 
            {
                _processOutputStreamAsBinary();
            }

            {
                std::lock_guard l(_syncRuntime);
                _syncRuntime.data().errthread = std::make_unique<std::thread>([this]{
                        process::getProcessStream(_stream_error, _onError, &_error);
                        });
            }

            _onStart.exec();

            std::error_code e;
            lg("waiting for process to finish...");
            
            bp::child* p;
            {
                std::lock_guard l(_syncRuntime);
                if (_syncRuntime.data().process)
                    p = _syncRuntime.data().process.get();
            }

            //this can't be locked in a mutex because, if not none of the runtime variable could be called during the entire process execution.
            p->wait(e);

            lg("Process finished.");

            {
                std::lock_guard l(_syncRuntime);
                // Join output threads
                if (_syncRuntime.data().outthread && _syncRuntime.data().outthread->joinable())
                {
                    lg("Joining out thread.");
                    _syncRuntime.data().outthread->join();
                    _syncRuntime.data().outthread.reset();
                }

                if (_syncRuntime.data().errthread && _syncRuntime.data().errthread->joinable())
                {
                    lg("Joining err thread.");
                    _syncRuntime.data().errthread->join();
                    _syncRuntime.data().errthread.reset();
                }
            }

            int exit_code = 0;
            {
                std::lock_guard l(_syncRuntime);
                if (_syncRuntime.data().process)
                    exit_code = _syncRuntime.data().process->exit_code();
                _syncRuntime.data().running = false;
            }

            lg("Process finished or killed.");

            if (exit_code != 0)
            {
                lg("Process error : " << e.message() << " and code : " << e.value());
                _processError = e.message();
                _onProcessError.exec();
            }

            _onEnd.exec();
        }
        catch(const std::exception& e)
        {
            lg("Process error : " << e.what());
            lg("command tried : " << this->cmd_s());
            _processError = e.what();
            _onProcessError.exec();
            
            {
                std::lock_guard l(_syncRuntime);
                _syncRuntime.data().running = false;
            }
        }
    };

    {
        std::lock_guard l(_syncRuntime);
        _syncRuntime.data().runthread = std::make_unique<std::thread>(run_func);
    }
}

void Process::_processOutputStreamAsBinary()
{
    auto f = [this]
    {
        std::vector<char> buf(_outBinaryBufSize);
        
        while (true)
        {
            {
                std::lock_guard l(_syncRuntime);
                if (!_syncRuntime.data().running)
                    break;
            }

            _stream_output.read(buf.data(), _outBinaryBufSize);
            std::streamsize bytes_read = _stream_output.gcount();
            
            if (bytes_read > 0)
            {
                std::vector<unsigned char> data(buf.begin(), buf.begin() + bytes_read);
                for (auto& cb : _onOutputBin)
                    cb(data);
            }
            else
            {
                break;
            }
        }
    };

    {
        std::lock_guard l(_syncRuntime);
        _syncRuntime.data().outthread = std::make_unique<std::thread>(f);
    }
}

void Process::terminate(bool sigkill)
{
    std::lock_guard l(_syncRuntime);
    
    if (!_syncRuntime.data().running || !_syncRuntime.data().process)
        return;

    lg("Process::terminate");
    
    if (sigkill)
        ::kill(_syncRuntime.data().process->id(), SIGKILL);
    else 
        _syncRuntime.data().process->terminate();
    
    _syncRuntime.data().running = false;
    
    // Note: We don't join threads here to avoid deadlock in destructor
    // The destructor will join them after releasing the lock
}

std::string Process::output()
{
    th_guard(_output);
    return _output.data();
}

std::string Process::error()
{
    th_guard(_error);
    return _error.data();
}

std::string Process::processError()
{
    return _processError;
}

void Process::write(const std::string& string)
{
    lg("a");
    {
    lg("a");
        th_guard(_syncRuntime);
    lg("a");
        if (!_syncRuntime.data().running)
            throw std::runtime_error("The process does not run anymore !");
    lg("a");
    }

    lg("a");
    {
    lg("a");
        th_guard(_stream_input);
    lg("a");
        _stream_input.data() << string << std::endl;
    lg("a");
    }
}

void Process::wrapInScript()
{
    _checker.check();
    _cmd = process::wrappedInScript(_cmd);
}

void process::getProcessStream(bp::ipstream& stream, const ml::Vec<std::function<void(const std::string& line)>>& cbs, th::Safe<std::string>* out)
{
    char c;
    std::string line;
    while (stream.get(c)) {
        if (c == '\n' || c == '\r') {
            // Process the completed line
            for (const auto& f : cbs) {
                if (f)
                    f(line);
            }

            {
                if (out)
                {
                    th_guard(*out);
                    out->data() += line + "\n";
                }
            }

            line.clear();

            // Skip the next character if it's part of a \r\n sequence
            if (c == '\r' && stream.peek() == '\n') {
                stream.get(); // Consume the \n
            }
        } else {
            line += c;
        }
    }

    // Don't forget to process the last line if it doesn't end with a newline
    if (!line.empty()) {
        for (const auto& f : cbs) {
            if (f)
                f(line);
        }

        {
            if (out)
            {
                th_guard(*out);
                out->data() += line;
            }
        }
    }
}

namespace args
{
    std::map<std::string, std::string> nparse(const int& argc, char* argv[])
    {
        std::map<std::string, std::string> _options;
        int nullOptionIdx = 0;
        for (int i=0; i<argc; i++)
        {
            if (i == 0)
                continue;

            std::string arg = std::string(argv[i]);
            if (arg[0] == '-' && i < argc - 1 && argv[i + 1][0] != '-')
            {
                i++;
                int idx = 1;
                if (arg[1] == '-')
                    idx = 2;
                _options[arg.substr(idx)] = std::string(argv[i]);
                lg("arg['" + arg.substr(idx) + "'] = " + argv[i]);
            }
            else if (arg[0] == '-' && ( i == argc - 1 || argv[i + 1][0] == '-'))
            {
                int idx = 1;
                if (arg[1] == '-')
                    idx = 2;
                _options[arg.substr(idx)] = std::string(argv[i]);
                lg("arg['" + arg.substr(idx) + "'] = " + argv[i]);
            }
            else 
            {
                _options[std::to_string(nullOptionIdx)] = arg;
                lg("arg['" + std::to_string(nullOptionIdx) + "'] = " + arg);
                nullOptionIdx ++;
            }
        }
        return _options;
    }

    std::map<std::string, std::string> parse(const int& argc, char* argv[])
    {
        return args::nparse(argc, argv);
    }

    //thank you chatGPT :)
    std::vector<std::string> simpleParse(const std::string& cmd)
    {
        std::vector<std::string> arguments;
        std::string current_arg;
        bool in_double_quote = false;
        bool in_single_quote = false;
        bool escape_next = false;

        for (char c : cmd) {
            if (escape_next) {
                current_arg += c;
                escape_next = false;
                continue;
            }

            if (c == '\\') {
                escape_next = true;
                continue;
            }

            if (c == '"' && !in_single_quote) {
                in_double_quote = !in_double_quote;
                continue;
            }

            if (c == '\'' && !in_double_quote) {
                in_single_quote = !in_single_quote;
                continue;
            }

            if (c == ' ' && !in_double_quote && !in_single_quote) {
                if (!current_arg.empty()) {
                    arguments.push_back(current_arg);
                    current_arg.clear();
                }
                continue;
            }

            current_arg += c;
        }

        // Push the last argument, if any.
        if (!current_arg.empty()) {
            arguments.push_back(current_arg);
        }

        return arguments;
    }
}


std::string process::exec(const char* cmd, const char* workingdir)
{
    std::string result = "";
    lg("trying to exec : " << cmd);
    if (workingdir)
        lg("from dir : " << workingdir);
#ifdef __linux__
    if (workingdir)
    {
        if (chdir(workingdir) != 0)
            lg("Error setting the dir " << workingdir);
    }
    char buffer[128];

    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
            result += buffer;
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;

#elif _WIN32
    bp::ipstream stream;
    std::string wd;
    if (!workingdir)
        wd = os::home();
    else
        wd = std::string(workingdir);
    bp::child c(std::string(cmd), bp::start_dir(wd), bp::std_out > stream);

    std::string line;
    while (getline(stream, line)) {
        // TODO likely post to some kind of queue for processing
        result += line + "\n";
    }

    c.wait(); // reap PID
    return result;
#endif
}

std::string process::exec(const std::string& cmd, const std::string& workingdir)
{
    std::string _r;
    if (workingdir.empty())
        _r = process::exec(cmd.c_str(), nullptr);
    else 
        _r = process::exec(cmd.c_str(), workingdir.c_str());
    return _r;
}

std::string process::exec(const std::vector<std::string>& cmd, const std::string& workingdir)
{
    std::string _r;
    return process::exec(process::to_string(cmd), workingdir);
    return _r;
}

std::vector<std::string> process::parse(const std::string& cmd)
{
    std::vector<std::string> args;
    std::string currentArg;
    bool inQuotes = false;
    bool escaped = false;

    for (char c : cmd) 
    {
        if (escaped) 
        {
            currentArg += c;
            escaped = false;
        }
        else if (c == '\\')
            escaped = true;
        else if (c == '"')
            inQuotes = !inQuotes;
        else if (c == ' ' && !inQuotes)
        {
            if (!currentArg.empty()) 
            {
                args.push_back(currentArg);
                currentArg.clear();
            }
        }
        else 
            currentArg += c;
    }

    if (!currentArg.empty()) {
        args.push_back(currentArg);
    }

    return args;
}

std::string process::to_string(const std::vector<std::string>& cmd)
{
    std::ostringstream command;
    for (size_t i = 0; i < cmd.size(); ++i) {
        if (i > 0) {
            command << " ";
        }
        
        // Check if we need to quote the argument
        bool needsQuotes = cmd[i].empty() || 
                          cmd[i].find_first_of(" \t\n\v\"'\\&|<>(){}[];$`") != std::string::npos;
        
        if (needsQuotes) {
            command << '"';
            
            // Escape any double quotes or backslashes
            for (char c : cmd[i]) {
                if (c == '"' || c == '\\') {
                    command << '\\';
                }
                command << c;
            }
            
            command << '"';
        } else {
            command << cmd[i];
        }
    }
    
    return command.str();
}

std::string process::exec(const std::string& cmd, std::string inData, const std::string& workingdir, const bp::environment& env, std::string* error)
{
    bp::ipstream out_s;
    bp::ipstream err_s;
    bp::opstream in_s;
    bp::environment _env = env;

    bp::child c(cmd, bp::start_dir(workingdir), bp::std_out > out_s, bp::std_err > err_s, bp::std_in < in_s, _env);
    if (!inData.empty())
    {
        in_s.write(inData.c_str(), inData.size());
        in_s.flush();
    }
    in_s.pipe().close(); // really importtant

    std::string _r;
    std::string line;
    while (out_s && std::getline(out_s, line))
        _r += line + "\n";

    if (error)
    {
        while (err_s && std::getline(err_s, line))
            *error += line + "\n";
    }

    c.wait();
    return _r;
}

namespace process
{
    ml::Vec<std::unique_ptr<Process>> _pcs; // always alive ? YES for now (FIXME only if it become a problem.)
}

Process* process::start(const std::string& cmd, const std::string& workingdir)
{
    auto p = std::make_unique<Process>(cmd);
    if (!workingdir.empty())
        p->setCwd(workingdir);

    p->start();
    _pcs.push(std::move(p));
    return _pcs.back().get();
}

Process* process::start(const std::string& cmd, std::function<void ()> onDoned, const std::string& workingdir)
{
    auto p = std::make_unique<Process>(cmd);
    if (!workingdir.empty())
        p->setCwd(workingdir);
    auto f = [onDoned]()
    {
        lg("Excecuting cb function for process end...");
        if (onDoned)
            onDoned();
    };
    p->addOnEnd(f);

    lg("aaaaaaaa");
    lg("Process cmd : " << p->cmd_s());
    p->start();

    _pcs.push(std::move(p));
    return _pcs.back().get();
}

Process* process::launch(const std::string& cmd, const std::string& workingdir)
{
    return process::start(cmd, workingdir);
}

Process* process::launch(const std::string& cmd, std::function<void ()> onDoned, const std::string& workingdir)
{
    return process::start(cmd, onDoned, workingdir);
}

std::string process::bashparsed(std::string data)
{
    data = str::replace(data, "\\", "\\\\");
    data = str::replace(data, " ", "\\ ");
    data = str::replace(data, "~", "\\~");
    data = str::replace(data, "`", "\\`");
    data = str::replace(data, "#", "\\#");
    data = str::replace(data, "$", "\\$");
    data = str::replace(data, "&", "\\&");
    data = str::replace(data, "*", "\\*");
    data = str::replace(data, "(", "\\(");
    data = str::replace(data, ")", "\\)");
    data = str::replace(data, "|", "\\|");
    data = str::replace(data, "[", "\\[");
    data = str::replace(data, "]", "\\]");
    data = str::replace(data, "{", "\\{");
    data = str::replace(data, "}", "\\}");
    data = str::replace(data, ";", "\\;");
    data = str::replace(data, "'", "\\'");
    data = str::replace(data, "\"", "\\\"");
    data = str::replace(data, "<", "\\<");
    data = str::replace(data, ">", "\\>");
    //data = str::replace(data, "/", "\\/");
    data = str::replace(data, "?", "\\?");
    data = str::replace(data, "!", "\\!");

    return data;
}

void process::chain(const std::vector<Process*>& processes, std::function<void ()> onDoned)
{
    if (processes.size() == 0)
        throw error::size("The processes list is empty.");
    if (processes.size() > 1)
    {
        for (int i=0; i<processes.size()-1; i++)
        {
            processes[i]->addOnEnd([=](){
                        processes[i+1]->start();
                    });
        }
    }

    if (onDoned)
        vc::last(processes)->addOnEnd(onDoned);

    processes[0]->start();
}

#ifdef __linux__
pid_t process::currentId()
{
    return getpid();
}

#elif _WIN32
DWORD process::currentId()
{
    return GetCurrentProcessId();
}
#endif

void process::end(int pgrId)
{
    lg2("ending process", pgrId);
    if (pgrId == 0)
        throw std::runtime_error("Can't end a programm with a pid of 0");
#ifdef __linux__
    ::kill(pgrId, SIGTERM);
#else 
    throw std::runtime_error("process::end not supported for this platform");
#endif
}

void process::kill(int pgrId)
{
    if (pgrId == 0)
        throw std::runtime_error("Can't kill a programm with a pid of 0");
#ifdef __linux__
    ::kill(pgrId, SIGKILL);
#else
    throw std::runtime_error("process::kill not supported for this platform");
#endif
}

#ifdef _WIN32
BOOL CALLBACK setProcessWinFG(HWND hwnd, LPARAM lParam)
{
    DWORD lpdwProcessId;
    GetWindowThreadProcessId(hwnd, &lpdwProcessId);
    if (lpdwProcessId == lParam)
    {
        SetForegroundWindow(hwnd);
        return FALSE;
    }
    return TRUE;
}
#endif

int process::system(const std::string& cmd)
{
#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = FALSE; // this hides the window

    if (!CreateProcessA(NULL,   // No module name (use command line)
        const_cast<char *>(cmd.c_str()), // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        CREATE_NO_WINDOW, // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
    )
    {
        MessageBoxA(nullptr, "The process could not be started...", "Error", MB_OK);
        return 1;
    }

    if (pi.hProcess != NULL)
    {
        // Wait for a while to ensure the new process's window is created
        Sleep(100);

        // Enumerate all top-level windows to find the one wich is swLauncher and bring it to top.
        EnumWindows(setProcessWinFG, pi.dwProcessId);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
#else 
    return std::system(cmd.c_str());
#endif
}

#ifdef _WIN32
std::wstring stringToWString(const std::string& s) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &wstr[0], size_needed);
    return wstr;
}
#endif

#ifdef _WIN32
std::string lastErrorAsString() {
    // Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        return std::string(); // No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, // unused with FORMAT_MESSAGE_FROM_SYSTEM
        errorMessageID,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL
    );

    std::string message(messageBuffer, size);

    // Free the buffer allocated by the system
    LocalFree(messageBuffer);

    return message;
}
#endif

void process::spawn(const std::string& cmd, const std::string& logFile)
{
#ifdef _WIN32
   STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // const char* to char* conversion
#ifdef _UNICODE
#else
    std::string cmd2 = cmd;
#endif
    
    // Start the child process. 
    if (!CreateProcess(NULL,   // No module name (use command line)
#ifdef _UNICODE
        const_cast<LPWSTR>(stringToWString(cmd).c_str()),              // Command line
#else
        const_cast<char*>(cmd2.c_str()),              // Command line
#endif
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        0,                      // No creation flags
        NULL,                   // Use parent's environment block
        NULL,                   // Use parent's starting directory 
        &si,                    // Pointer to STARTUPINFO structure
        &pi)                    // Pointer to PROCESS_INFORMATION structure
        )
        throw std::runtime_error("process::spawn - CreateProcess failed: " + lastErrorAsString() + "\nFor the command : " + cmd);

    // Parent process can continue or exit. The child will run independently.
    lg("Child process spawned : " + cmd);

    // Optionally, if you want to wait for the child process to complete, uncomment the line below:
    // WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles to cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else 
    // Fork off the parent process
    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("process::spawn : fork failed.");
    else if (pid>0) // i'm in parent process
    {
        lg("I'm the parent, so I continuing my life.");
        return;
    }
    else if (pid == 0) // i'm in child process
    {
        if (setsid() < 0)
        {
            perror("Failed to create a new session setsid");
            exit(EXIT_FAILURE);
        }

        if (logFile.empty())
        {
            // Close out the standard file descriptors (stdin, stdout, stderr)
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }
        else // log the stdout and the stderr in a file using the unix api directly. Be careful this won't work on Window but this should never be executed on window so you should be fine.
        {
            int logFileId = open(logFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if(logFileId < 0) {
                perror("Failed to open log file");
                exit(EXIT_FAILURE);
            }

            dup2(logFileId, STDOUT_FILENO);
            dup2(logFileId, STDERR_FILENO);
            close(logFileId);
        }

        // Replace the child process image with the desired program.
        lg2("spawning", cmd);
        auto arguments = args::simpleParse(cmd);
        auto args = str::fromStringList(arguments);
        execvp(arguments[0].c_str(), const_cast<char* const*>(args));
    }
#endif
}
std::string process::onlyReadables(const std::string& input) {
    // This regex matches:
    // 1. ANSI color/style codes (like \033[31m for red)
    // 2. ANSI cursor movement codes
    // 3. Other common terminal control sequences
    static const std::regex ansiPattern("\033\\[(\\d*;)*(\\d*)m|"      // Color codes
                                       "\033\\[\\d*[A-Za-z]|"          // Cursor movement
                                       "\033\\[[0-9;]*[a-zA-Z]|"       // Other ANSI sequences
                                       "\033\\]\\d*;.*?\\007|"         // OSC sequences (title, etc.)
                                       "\r|"                          // Carriage return
                                       "\033\\[\\?\\d+[hl]");         // Terminal mode settings

    // Replace all matching sequences with empty string
    return std::regex_replace(input, ansiPattern, "");
}

std::vector<std::string> process::wrappedInScript(const std::vector<std::string>& cmd)
{
    std::string c = process::to_string(cmd);	
    return {"/usr/bin/script", "-q", "-c", c, "/dev/null"};
}

