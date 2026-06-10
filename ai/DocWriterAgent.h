#pragma once
#include "./FilesWriterAgent.h"

namespace ml
{
    class DocWriterAgent : public FilesWriterAgent
    {
        public:
            DocWriterAgent(const std::string& name="");
    };
}