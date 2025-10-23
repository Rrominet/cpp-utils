#include "./process-launcher.h"
#include "network/TcpClient.h"

namespace pl
{
    void spawn(const std::string& cmd)
    {
        TcpClient tcp("localhost:50043");
        tcp.send(cmd, false);
    }
}
