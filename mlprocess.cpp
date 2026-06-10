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
        {
            std::lock_guard l(_processErrorMutex);
            _processError = "";
        }
    };

    this->addOnEnd(reset_outputs);
    this->addOnTerminate(reset_outputs);


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
    // Signal the process to die
    {
        std::lock_guard l(_syncRuntime);
        if (_syncRuntime.data().running && _syncRuntime.data().process)
        {
            ::kill(_syncRuntime.data().process->id(), SIGKILL);
            // Mark running false so run_func doesn't re-lock and fight us
            _syncRuntime.data().running = false;
        }
    }

    // Close streams so blocked reads in out/err threads will return
    if (_stream_output.pipe().is_open())
        _stream_output.pipe().close();
    if (_stream_error.pipe().is_open())
        _stream_error.pipe().close();
    {
        std::lock_guard l(_stream_input);
        if (_stream_input.data().pipe().is_open())
            _stream_input.data().pipe().close();
    }

    _joinThreads();
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

    std::unique_ptr<std::thread> outth;
    std::unique_ptr<std::thread> errth;
    std::unique_ptr<std::thread> runth;

    {
        std::lock_guard l(_syncRuntime);
        outth = std::move(_syncRuntime.data().outthread);
        errth = std::move(_syncRuntime.data().errthread);
        runth = std::move(_syncRuntime.data().runthread);
    }

    // Join out/err first (they should finish quickly once pipes are closed)
    if (outth && outth->joinable())
        outth->join();
    if (errth && errth->joinable())
        errth->join();
    // Join run thread last
    if (runth && runth->joinable())
        runth->join();
}

void Process::addOnStart(std::function<void()> f)
{
    std::lock_guard l(_cbsMtx);
    _onStart.push(f);
}

unsigned int Process::addOnOutput(const std::function<void(const std::string& line)> &f)
{
    std::lock_guard l(_cbsMtx);
    _onOutput.push(f);
    return _onOutput.size() - 1;
}

unsigned int Process::addOnOutputBin(const std::function<void(const std::vector<unsigned char>& data_chunk)> &f)
{
    std::lock_guard l(_cbsMtx);
    _onOutputBin.push(f);
    return _onOutputBin.size() - 1;
}

void Process::addOnError(const std::function<void(const std::string& line)> &f)
{
    std::lock_guard l(_cbsMtx);
    _onError.push(f);
}

void Process::addOnEnd(const std::function<void()> &f)
{
    std::lock_guard l(_cbsMtx);
    _onEnd.push(f);
}

void Process::addOnTerminate(const std::function<void()> &f)
{
    std::lock_guard l(_cbsMtx);
    _onTerminate.push(f);
}

void Process::addOnProcessError(const std::function<void()> &f)
{
    std::lock_guard l(_cbsMtx);
    _onProcessError.push(f);
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


        {
            std::lock_guard l(_stream_input);
            _stream_input.data() = bp::opstream();
        }
        _stream_output = bp::ipstream();
        _stream_error = bp::ipstream();

        try
        {
            {
                std::lock_guard l(_syncRuntime);
                std::lock_guard l2(_stream_input);
#ifdef _WIN32
                _syncRuntime.data().process = std::make_unique<bp::child>(_cmd, bp::start_dir(_cwd), bp::std_out > _stream_output,
                                                       bp::std_err > _stream_error, bp::std_in < _stream_input.data(),
                                                       bp::windows::create_no_window);
#else
                _syncRuntime.data().process = std::make_unique<bp::child>(_cmd, bp::start_dir(_cwd), bp::std_out > _stream_output,
                                                       bp::std_err > _stream_error, bp::std_in < _stream_input.data());
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
                    process::getProcessStream(_stream_output, _onOutput, &_output, &_cbsMtx);
                });
            }
            else
            {
                _processOutputStreamAsBinary();
            }

            {
                std::lock_guard l(_syncRuntime);
                _syncRuntime.data().errthread = std::make_unique<std::thread>([this]{
                    process::getProcessStream(_stream_error, _onError, &_error, &_cbsMtx);
                });
            }

            {
                std::lock_guard l(_cbsMtx);
                _onStart.exec();
            }

            std::error_code e;
            lg("waiting for process to finish...");

            // Wait without holding any lock — use a local shared_ptr-guarded copy
            // to avoid use-after-free if terminate() resets the process ptr.
            // We keep the bp::child alive for the wait by copying the pid and
            // calling wait directly on the managed object under a try/catch.
            // Safe: terminate() sets running=false and kills the pid but does NOT
            // reset/delete the unique_ptr, so the pointer remains valid until
            // run_func itself resets it below.
            bp::child* p = nullptr;
            {
                std::lock_guard l(_syncRuntime);
                p = _syncRuntime.data().process.get();
            }

            if (p)
                p->wait(e);

            lg("Process finished.");

            // Join out/err threads — pipes are now closed/EOF
            std::unique_ptr<std::thread> outth;
            std::unique_ptr<std::thread> errth;
            {
                std::lock_guard l(_syncRuntime);
                outth = std::move(_syncRuntime.data().outthread);
                errth = std::move(_syncRuntime.data().errthread);
            }
            if (outth && outth->joinable())
            {
                lg("Joining out thread.");
                outth->join();
            }
            if (errth && errth->joinable())
            {
                lg("Joining err thread.");
                errth->join();
            }

            int exit_code = 0;
            bool was_running = false;
            {
                std::lock_guard l(_syncRuntime);
                was_running = _syncRuntime.data().running;
                if (_syncRuntime.data().process)
                    exit_code = _syncRuntime.data().process->exit_code();
                _syncRuntime.data().running = false;
                // Reset process so destructor doesn't kill an already-dead pid
                _syncRuntime.data().process.reset();
            }

            lg("Process finished or killed.");

            if (!was_running)
            {
                // terminate() was called — fire onTerminate callbacks
                std::lock_guard l(_cbsMtx);
                _onTerminate.exec();
                return;
            }

            if (exit_code != 0)
            {
                lg("Process error code : " << exit_code);
                {
                    std::lock_guard l(_processErrorMutex);
                    _processError = e.message();
                }
                {
                    std::lock_guard l(_cbsMtx);
                    _onProcessError.exec();
                }
            }

            {
                std::lock_guard l(_cbsMtx);
                _onEnd.exec();
            }
        }
        catch(const std::exception& e)
        {
            lg("Process error : " << e.what());
            lg("command tried : " << this->cmd_s());
            {
                std::lock_guard l(_processErrorMutex);
                _processError = e.what();
            }
            {
                std::lock_guard l(_cbsMtx);
                _onProcessError.exec();
            }

            {
                std::lock_guard l(_syncRuntime);
                _syncRuntime.data().running = false;
                _syncRuntime.data().process.reset();
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
    // Don't check running flag before blocking read — instead rely on the pipe
    // being closed (by terminate/destructor) to unblock the read naturally.
    auto f = [this]
    {
        std::vector<char> buf(_outBinaryBufSize);

        while (_stream_output.good())
        {
            _stream_output.read(buf.data(), _outBinaryBufSize);
            std::streamsize bytes_read = _stream_output.gcount();

            if (bytes_read > 0)
            {
                std::vector<unsigned char> data(buf.begin(), buf.begin() + bytes_read);

                ml::Vec<std::function<void(const std::vector<unsigned char>& data_chunk)>> cbs;
                {
                    std::lock_guard l(_cbsMtx);
                    cbs = _onOutputBin;
                }
                for (auto& cb : cbs)
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
    int pid = 0;
    {
        std::lock_guard l(_syncRuntime);

        if (!_syncRuntime.data().running || !_syncRuntime.data().process)
            return;

        lg("Process::terminate");
        pid = _syncRuntime.data().process->id();
        // Mark not running BEFORE killing so run_func's wait() return path
        // sees was_running==false and fires onTerminate instead of onEnd.
        _syncRuntime.data().running = false;
    }

    // Kill outside the lock so run_func can re-acquire it after wait() returns
    if (pid > 0)
    {
        if (sigkill)
            ::kill(pid, SIGKILL);
        else
            ::kill(pid, SIGTERM);
    }

    // Close pipes so blocked out/err reader threads unblock immediately
    if (_stream_output.pipe().is_open())
        _stream_output.pipe().close();
    if (_stream_error.pipe().is_open())
        _stream_error.pipe().close();
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
    std::lock_guard l(_processErrorMutex);
    return _processError;
}

void Process::write(const std::string& string)
{
    {
        th_guard(_syncRuntime);
        if (!_syncRuntime.data().running)
            throw std::runtime_error("The process does not run anymore !");
    }

    {
        th_guard(_stream_input);
        _stream_input.data() << string << std::endl;
    }
}

void Process::wrapInScript()
{
    _cmd = process::wrappedInScript(_cmd);
}

void process::getProcessStream(bp::ipstream& stream, const ml::Vec<std::function<void(const std::string& line)>>& cbs, th::Safe<std::string>* out, std::mutex* cb_mtx)
{
    char c;
    std::string line;
    while (stream.get(c))
    {
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
    if (!line.empty())
    {
        ml::Vec<std::function<void(const std::string& line)>> cbs_cp;
        if (cb_mtx)
        {
            th_guard(*cb_mtx);
            cbs_cp = cbs;
        }
        else 
            cbs_cp = cbs;
        for (const auto& f : cbs_cp)
        {
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
                if (arg.size() > 1 && arg[1] == '-')
                    idx = 2;
                // Flag-only arg: store empty string, not the flag name itself
                _options[arg.substr(idx)] = "";
                lg("arg['" + arg.substr(idx) + "'] = (flag)");
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


ProcessOut process::exec2(const char* cmd, const char* workingdir)
{
    ProcessOut result;
    bp::ipstream out_s;
    bp::ipstream err_s;

    std::string wd = workingdir ? std::string(workingdir) : os::home();

    bp::child c(std::string(cmd), bp::start_dir(wd), bp::std_out > out_s, bp::std_err > err_s);

    // Read stdout and stderr concurrently to avoid pipe-buffer deadlock
    std::string out_buf, err_buf;
    std::thread err_reader([&]{
        std::string line;
        while (err_s && std::getline(err_s, line))
            err_buf += line + "\n";
    });

    std::string line;
    while (out_s && std::getline(out_s, line))
        out_buf += line + "\n";

    lg("joining error reader...");
    err_reader.join();

    lg("waiting for the process to finish...");
    c.wait();
    lg("process done.");
    result.stdout = out_buf;
    result.stderr = err_buf;
    result.exitCode = c.exit_code();
    return result;
}

ProcessOut process::exec2(const std::string& cmd, const std::string& workingdir)
{
    return process::exec2(cmd.c_str(), workingdir.empty() ? nullptr : workingdir.c_str());
}

ProcessOut process::exec2(const std::vector<std::string>& cmd, const std::string& workingdir)
{
    return process::exec2(process::to_string(cmd), workingdir);
}


std::string process::exec(const char* cmd, const char* workingdir)
{
    return process::exec2(cmd, workingdir).stdout;
}

std::string process::exec(const std::string& cmd, const std::string& workingdir)
{
    return process::exec2(cmd, workingdir).stdout;
}

std::string process::exec(const std::vector<std::string>& cmd, const std::string& workingdir)
{
    return process::exec2(cmd, workingdir).stdout;
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
    
    std::string c = command.str();
    c = str::replace(c, "\"<<\"", "<<");
    c = str::replace(c, "\"<\"", "<");
    c = str::replace(c, "\">>\"", ">>");
    c = str::replace(c, "\"&\"", "&");
    return c;
}

std::string process::exec(const std::string& cmd, std::string inData, const std::string& workingdir, const bp::environment& env, std::string* error)
{
    bp::ipstream out_s;
    bp::ipstream err_s;
    bp::opstream in_s;
    bp::environment _env = env;

    bp::child c(cmd, bp::start_dir(workingdir), bp::std_out > out_s, bp::std_err > err_s, bp::std_in < in_s, _env);

    // Write stdin in a separate thread to avoid blocking if the child doesn't
    // drain stdin before filling its stdout/stderr buffers.
    std::thread stdin_writer([&]{
        if (!inData.empty())
        {
            in_s.write(inData.c_str(), inData.size());
            in_s.flush();
        }
        in_s.pipe().close();
    });

    // Read stderr concurrently to avoid deadlock
    std::string err_buf;
    std::thread err_reader([&]{
        std::string line;
        while (err_s && std::getline(err_s, line))
            err_buf += line + "\n";
    });

    std::string _r;
    std::string line;
    while (out_s && std::getline(out_s, line))
        _r += line + "\n";

    stdin_writer.join();
    err_reader.join();

    if (error)
        *error = err_buf;

    c.wait();
    return _r;
}

namespace process
{
    ml::Vec<std::unique_ptr<Process>> _pcs;
    std::mutex _pcsMutex;
}

void process::cleanupFinished()
{
    std::lock_guard l(_pcsMutex);
    _pcs.vec.erase(std::remove_if(_pcs.begin(), _pcs.end(), [](const std::unique_ptr<Process>& p){
        return p && !p->running();
    }), _pcs.end());
}

Process* process::start(const std::string& cmd, const std::string& workingdir)
{
    auto p = std::make_unique<Process>(cmd);
    if (!workingdir.empty())
        p->setCwd(workingdir);

    // Schedule cleanup of finished processes before adding new ones
    {
        std::lock_guard l(_pcsMutex);
        _pcs.vec.erase(std::remove_if(_pcs.begin(), _pcs.end(), [](const std::unique_ptr<Process>& p){
            return p && !p->running();
        }), _pcs.end());
    }

    p->start();

    std::lock_guard l(_pcsMutex);
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

    {
        std::lock_guard l(_pcsMutex);
        _pcs.vec.erase(std::remove_if(_pcs.begin(), _pcs.end(), [](const std::unique_ptr<Process>& p){
            return p && !p->running();
        }), _pcs.end());
    }

    lg("Process cmd : " << p->cmd_s());
    p->start();

    std::lock_guard l(_pcsMutex);
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
        waitpid(pid, nullptr, 0);
        return;
    }
    else if (pid == 0) // i'm in child process
    {
        pid_t pid2 = fork();
        if (pid2 == 0)
        {
            // Grandchild - will be adopted by init
            if(setsid() < 0)
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

            //FIXME : not ture the special chars and "" will work here ...
            execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
            _exit(1);
        }
        _exit(0);
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

