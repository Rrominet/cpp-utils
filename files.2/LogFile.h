#pragma once
#include "./files.h"
#include <mutex>

namespace ml
{
    class LogFile
    {
        public:
            LogFile(const std::string& filepath);
            void write(std::string tolog);
            bool async() const;
            void setAsync(bool async);
            std::string filepath() const;
            void setFilepath(const std::string& filepath);

            virtual ~LogFile(){}

        private : 
            std::string _filepath;
            bool _async = true;
            mutable std::mutex _mutex;
    };
}
