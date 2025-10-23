#pragma once
#include <iostream>
#include <cstdlib>
#include <unordered_map>

namespace apache
{
    std::string ip();
    class Connection
    {
        private : 
            std::string _ip = "";
            std::string _filepath = "";
            bool _oppened = false;
        public : 
            void open(const std::string &dir);
            void close();
    };

    // return the current number of connections from the same ip
    // the dir is where the connections are stored
    int connections(const std::string &dir);


    // minTime is time in seconds 
    // if the connection has been modified after the time (modifed>= time() - minTime) so the connection wont be reset.
    bool reset(const std::string &dir, const int minTime=0);
    bool log();
    bool logout();

    void overrideIp(const std::string& ip);

    std::unordered_map<std::string, std::string> GET();
}
