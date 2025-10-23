#include "./mlprocess.h"
#include "str.h"
#include "os.h"
#include "exceptions.h"
#include <memory>
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

Process::Process() : _runmtx("Process Run Mutex"), _mtx("Process Output Mutex"), _emtx("Process Error Mutex"), _runthmtx("Process Run Thread Mutex")
{
    _init();
}

Process::Process(const std::string& cmd) : _cmd(process::parse(cmd)),_runmtx("Process Run Mutex"), _mtx("Process Output Mutex"), _emtx("Process Error Mutex"), _runthmtx("Process Run Thread Mutex")
{
    _init();
}

Process::Process(const std::string& cmd, const std::string& cwd) : _cmd(process::parse(cmd)), _cwd(cwd),_runmtx("Process Run Mutex"), _mtx("Process Output Mutex"), _emtx("Process Error Mutex"), _runthmtx("Process Run Thread Mutex")
{
    _init();
}

Process::Process(const std::vector<std::string>& cmd) : _cmd (cmd),_runmtx("Process Run Mutex"), _mtx("Process Output Mutex"), _emtx("Process Error Mutex"), _runthmtx("Process Run Thread Mutex")
{
    _init();
}

Process::Process(const std::vector<std::string>& cmd,const std::string& cwd) : _cmd(cmd), _cwd(cwd),_runmtx("Process Run Mutex"), _mtx("Process Output Mutex"), _emtx("Process Error Mutex"), _runthmtx("Process Run Thread Mutex")
{
    _init();
}

void Process::_init()
{
    auto f = [this]
    {
        {
            th_guard(_mtx);
            _output = "";
        }
        {
            th_guard(_emtx);
            _error = "";
        }
        _processError = "";
    };

    this->addOnEnd(f);
    this->addOnTerminate(f);
}

Process::~Process()
{
   this->terminate();
   {
        std::lock_guard lk(_runthmtx);
       if (_runthread)
           _runthread->join();
   }
   if (_outthread)
       _outthread->join();
   if (_errthread)
       _errthread->join();

   _stream_input.pipe().close();
   _stream_output.pipe().close();
   _stream_error.pipe().close();

   lg("~Process");
}

void Process::addOnStart(std::function<void()> f)
{
    _onStart.push(f);
}

unsigned int  Process::addOnOutput(const std::function<void(const std::string& line)> &f)
{
    _onOutput.push(f);
    return _onOutput.size() - 1;
}

unsigned int Process::addOnOutputBin(const std::function<void(const std::vector<unsigned char>& data_chunk)> &f)
{
    _onOutputBin.push(f);
    return _onOutputBin.size() - 1;
}

void Process::addOnError(const std::function<void(const std::string& line)> &f)
{
    _onError.push(f);
}

void Process::setCmd_s(const std::string &cmd)
{
    lg("settings _cmd..");
    _cmd = process::parse(cmd);
    lg("_cmd setted : " + cmd);
}

std::string Process::cmd_s() const
{
    return process::to_string(_cmd);	
}

void Process::start()
{
    if (!_can_start) 
    {
        lg("_can_start is false. Abort." );
        return;
    }

    if (_running)
    {
        lg("_running is true. Abort." );
        return;
    }

    _can_start = false;

    if (_cmd.empty())
    {
        _can_start = true;
        throw error::missing_arg("cmd is empty ! No process to launch.");
    }

    if (_cwd.empty())
        _cwd = os::home();

    if (!files::isDir(_cwd))
    {
        _can_start = true;
        throw error::dir_not_found("current working dir is not a dir !");
    }

    auto f = [=] 
    {
        lg("starting process...");
        lg("Process " << this);
        lg("cmd : " + this->cmd_s());

        _stream_input = bp::opstream();
        _stream_output = bp::ipstream();
        _stream_error = bp::ipstream();

        try
        {
            {
                th_guard(_runmtx);
#ifdef _WIN32
                process = std::make_unique<bp::child>(_cwd), bp::std_out > _stream_output, bp::std_err > _stream_error, bp::std_in < _stream_input, bp::windows::create_no_window);
#else
                _process = std::make_unique<bp::child>(_cmd, bp::start_dir(_cwd), bp::std_out > _stream_output, bp::std_err > _stream_error, bp::std_in < _stream_input);
#endif
            }
            lg2("process", this->cmd_s());
            lg2("current working dir", _cwd);
            lg("process {" + this->cmd_s()  + "} just started in dir " + _cwd);

            if (!_treatOutputAsBinary)
            {
                _outthread = std::make_unique<std::thread>([this]{
                        process::getProcessStream(_stream_output, _onOutput, &_output, &_mtx);
                        });
            }
            else 
                _processOutputStreamAsBinary();

            _errthread = std::make_unique<std::thread>([this]{
                    process::getProcessStream(_stream_error, _onError, &_output, &_emtx);
            });

            std::error_code e;
            lg("wating for process to finish...");
            _process->wait(e);
            lg("Process finished.");
            lg("Joining out thread.");
            _outthread->join();
            _outthread.reset();

            lg("Joining err thread.");
            _errthread->join();
            _errthread.reset();
            lg("Err Thread joined.");

            _running = false;

            lg("Process finished or killed.");

            if (_process->exit_code() != 0)
            {
                lg("Process error : " << e.message() << " and code : " << e.value());
                _processError = e.message();
                _onProcessError.exec();
            }

            _can_start = true;
            _onEnd.exec();
        }
        catch(const std::exception& e)
        {
            lg("Process error : " << e.what());
            lg("command tried : " << this->cmd_s());
            _processError = e.what();
            _onProcessError.exec();
            _can_start = true;
        }
    };

    {
        std::lock_guard lk(_runthmtx);
        if (_runthread)
        {
            _runthread->join();
            _runthread.reset();
        }
    }

    _running = true;
    _runthread = std::make_unique<std::thread>(f);
}

void Process::_processOutputStreamAsBinary()
{
    auto f = [this]
    {
        char buf[_outBinaryBufSize];
        th_guard(_mtx);
        while (_stream_output.read(buf, _outBinaryBufSize))
        {
            if (!_running)
                break;

            for (auto& cb : _onOutputBin)
                cb(std::vector<unsigned char>(buf, buf + _outBinaryBufSize));

            if (!_running)
                break;
        }
    };

    _outthread = std::make_unique<std::thread>(f);
}

void Process::terminate(bool sigkill)
{
    if (!_running)
        return;

    lg("Process::terminate");
    {
        lg("trying to lock guard the _process var...");
        th_guard(_runmtx);
        lg("lock for _process aquired.");
        if (!_process)
        {
            _running = false;
            _can_start = true;
            return;
        }
        if (sigkill)
            ::kill(_process->id(), SIGKILL);
        else 
            _process->terminate();
    }

    lg("Process terminated.");
    {
        std::lock_guard lk(_runthmtx);
        if (_runthread)
        {
            _runthread->join();
            _runthread.reset();
        }
    }
    _onTerminate.exec();
}

std::string Process::output()
{
    {
        th_guard(_mtx);
        return _output;
    }
}

std::string Process::error()
{
    {
        th_guard(_emtx);
        return _error;
    }
}

std::string Process::processError()
{
    return _processError;
}

void Process::addOnEnd(const std::function<void()> &f)
{
   _onEnd.push(f);
}

void Process::addOnTerminate(const std::function<void()> &f)
{
    _onTerminate.push(f) ;
}

void Process::addOnProcessError(const std::function<void()> &f)
{
    _onProcessError.push(f) ;
}

void Process::write(const std::string& string)
{
    if (!_running)
        throw std::runtime_error("The process does not run anymore !");
    _stream_input << string << std::endl;
}

void Process::wrapInScript()
{
    _cmd = process::wrappedInScript(_cmd);
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

void process::getProcessStream(bp::ipstream& stream, const ml::Vec<std::function<void(const std::string& line)>>& cbs, std::string* out, ml::NamedMutex* mtx)
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
                if (mtx)
                    th_guard(*mtx);
                if (out)
                    *out += line + "\n";
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
            if (mtx)
                th_guard(*mtx);
            if (out)
                *out += line;
        }
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

