#include "./signals.h"
#include <signal.h>
#include <stdexcept>

namespace signals
{
    void connect(int sign, void (*handler)(int))
    {
        struct sigaction sa;
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(sign, &sa, NULL) < 0)
            throw std::runtime_error("Failed to connect to signal : " + std::to_string(sign));
    }

    void connectToAll(void (*func)(int))
    {
        connect(SIGINT, func);
        connect(SIGTERM, func);
        connect(SIGSEGV, func);
        connect(SIGABRT, func);
        connect(SIGHUP, func);
        connect(SIGQUIT, func);
        connect(SIGFPE, func);
        connect(SIGILL, func);
        connect(SIGBUS, func);
        connect(SIGPIPE, func);
        connect(SIGTSTP, func);
        connect(SIGXCPU, func);
        connect(SIGXFSZ, func);
        connect(SIGVTALRM, func);
        connect(SIGPROF, func);
        connect(SIGSYS, func);
    }
}
