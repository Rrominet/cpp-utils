#pragma once
#include <string>
#include <functional>
#include "../vec.h"
#include "./AgentConfig.h"
#include "../Ret.h"

class Process;
namespace ml
{
    class Agent
    {
        public:
            Agent(const std::string& id = "", const std::string& name="", const std::string& context = "") : _context(context), _id(id), _name(name){}

            const std::string name() const;

            //the string argument is the _inData;
            //the return string will be add if not empty in the _inData sended to the llm.
            void addBeforeLlmCall(const std::function<std::string(Agent*, const std::string&)>& f) {_beforeLlmCall.push_back(f);}

            //the 1st argument is the in data sent to the llm;
            //the 2nd argument is the data 
            //the return string is the data that gonna be available in _outData by other Agents.
            void addAfterLlmCall(const std::function<std::string(Agent*, const std::string&, const std::string&)>& f) {_afterLlmCall.push_back(f);}

            //this is full sync, it returns when the llm is done
            //at the end of this, all the _before and _after function hasbeen called and the _outData should be populated.
            //the llm process is typically the /opt/ia-wrap/ia-wrap process that works with ipc and that simply calls the llm
            Ret<> exec(Process* llm);

            json serialize() const;
            void deserialize(const json& config);

            void prependToContext(const std::string& context) { _context = context + "\n" + _context; }
            void appendToContext(const std::string& context) { _context += "\n" + context; }
            void addToContext(const std::string& context) { _context += "\n" + context; }

            void addProjectContext(const std::string& prj);

            Ret<> addProjectContextFile(std::string filepath);

            //the filepath is from the root or absolute
            void addFiles(const ml::Vec<std::string>& files);
            void addWritingRules(const std::string& rules);

        protected:
            AgentConfig _config; //bp cg
            std::string _inData; //bp cgs
            std::string _outData; //bp cg
            std::string _context; //bp cgs
            std::string _id; //bp cgs
            std::string _name; //bp s
            std::string _description; //bp cgs

            //if this is not empty the process stop (before or after the llm call)
            std::string _error; //bp cgs

            //project root, for the files
            std::string _root; //bp cgs
            std::string _rawllm; //bp cg

            //the string argument is the _inData;
            //the return string will be add if not empty in the _inData sended to the llm.
            ml::Vec<std::function<std::string(Agent*, const std::string&)>> _beforeLlmCall;

            //the 1st argument is the in data sent to the llm;
            //the 2nd argument is the data 
            //the return string is the data that gonna be available in _outData by other Agents.
            ml::Vec<std::function<std::string(Agent*, const std::string&, const std::string&)>> _afterLlmCall;

            void _writeWhatItSent();
            void _writeWhatItReceived(const std::string& received);
            void _writeLog(const std::string& msg);

        public : 
#include "./Agent_gen.h"

    };
}
