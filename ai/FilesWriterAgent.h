#pragma once
#include "./Agent.h"
#include "../Ret.h"

namespace ml
{
    enum ParserState {
        PARSER_NONE = 0,
        PARSER_SEARCH,
        PARSER_REPLACE,
        PARSER_CREATE,
        PARSER_APPEND,
        PARSER_NOTE
    };

    struct FilesWriterAction
    {
        ParserState action;
        std::string file;
        std::string searched;
        std::string content;
    };

    class OutputParser
    {
        public : 
            Ret<> writeCode(const std::string& iaout, const std::string& root);
            const ml::Vec<FilesWriterAction>& actions()const  {return _actions;};
            FilesWriterAction action(ParserState action,const std::string& file,const std::string& searched,const std::string& content, const std::string& note);

            const ml::Vec<std::string>& notes()const {return _notes;}

        private : 
            ParserState _state = PARSER_NONE;
            std::string _file = "";
            std::string _file_data = "";
            std::string _search = "";
            std::string _content = "";
            std::string _note = "";
            ml::Vec<std::string> _notes;

            ml::Vec<FilesWriterAction> _actions;
    };

    class FilesWriterAgent : public Agent
    {
        public:
            FilesWriterAgent(const std::string& name="");
            const ml::Vec<FilesWriterAction>& actions()const  {return _parser.actions();};
            const ml::Vec<std::string>& notes()const  {return _parser.notes();};
            std::string writeFiles(const std::string& inData, const std::string& outData);

        private:
            std::string _generateCtx();
            OutputParser _parser;
    };
}
