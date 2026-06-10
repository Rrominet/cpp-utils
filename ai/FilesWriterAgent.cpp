#include "./FilesWriterAgent.h"
#include "../str.h"
#include "../files.2/files.h"
#include "./utils.h"

namespace ml
{
    FilesWriterAgent::FilesWriterAgent(const std::string& name) : Agent("FilesWriterAgent", name)
    {
        this->setContext(_generateCtx());
        this->addAfterLlmCall([](Agent* self, const std::string& inData, const std::string& outData) -> std::string{
                    auto agent = static_cast<FilesWriterAgent*>(self);
                    return agent->writeFiles(inData, outData);
                });
    }

    std::string FilesWriterAgent::_generateCtx()
    {
        return R"(Your task is to create and write into files.
If the task asked is incoherant or it misses too much context to be performed, you can write notes explaining to the user what's missing.
You receive as input the task(s) to perform written in a natural language.
You output ONLY structured diff blocks. Nothing else.

DIFF FORMAT:

For modifying existing code:
<<<FILE: path/to/file>>>
<<<SEARCH>>>
[minimum 3 lines of existing unmodified code, must be unique in file]
<<<REPLACE_WITH>>>
[new code]
<<<END>>>

For creating a new file:
<<<FILE: path/to/file>>>
<<<CREATE>>>
[full file content]
<<<END>>>

For appending to end of file:
<<<FILE: path/to/file>>>
<<<APPEND>>>
[content to append]
<<<END>>>

For critical notes only:
<<<NOTE>>>
[your note]
<<<END>>>

RULES - NON NEGOCIABLE:
- Output ONLY diff blocks
- NEVER write anything outside blocks
- NEVER explain what you did
- NEVER confirm or summarize
- SEARCH block MUST contain at least 3 unmodified lines from the actual file
- SEARCH block MUST be unique within the file
- One response can contain multiple blocks across multiple files
- If what asked is impossible or incoherant only respond with a <<<NOTE>>> block explaining the problem, nothing else.
- Violation of this format makes your output unparseable and useless
        )";
    }

    std::string FilesWriterAgent::writeFiles(const std::string& inData, const std::string& outData)
    {
        lg("FilesWriterAgent::_writeFiles(" << inData << ", " << outData << ")");
        auto ret = _parser.writeCode(outData, this->root());
        if (!ret.success)
            this->setError(ret.message);
        return "";
    }

    FilesWriterAction OutputParser::action(ParserState action,const std::string& file,const std::string& searched,const std::string& content, const std::string& note)
    {
        lg("Creating a new Action : " << action << ", " << file << ", " << searched << ", " << content << ", " << note);
        FilesWriterAction a;
        a.action = action;
        a.file = file;
        a.searched = searched;
        a.content = content;
        if (!note.empty())
            a.content = note;
        return a;
    }

    Ret<> OutputParser::writeCode(const std::string& iaout,const std::string& root)
    {
        lg("OutputParser::writeCode(" << iaout << ", " << root << ")");
        std::string error;
        _actions.clear();
        _notes.clear();
        auto lines =  str::split(iaout, "\n");
        lg("Analyzing " << lines.size() << " lines");
        for (size_t i = 0; i < lines.size(); i++)
        {
            lg("Running on line " << i);
            if (str::contains(lines[i], "<<<FILE: "))
            {
                _file = str::replace(lines[i], "<<<FILE: ", "");
                _file = str::replace(_file, ">>>", "");
                try
                {
                    if (files::exists(root + files::sep() + _file))
                        _file_data = files::read(root + files::sep() + _file);
                    else 
                        _file_data = "";
                }
                catch(const std::exception& e)
                {
                    error += "Can't open to read file " + root + files::sep() + _file + " : " + std::string(e.what()) + "\n\n";
                    _file_data = "";
                }
            }

            else if (str::contains(lines[i], "<<<SEARCH>>>"))
            {
                _state = PARSER_SEARCH;
                _search = "";
                continue;
            }
            else if (str::contains(lines[i], "<<<REPLACE_WITH>>>"))
            {
                _state = PARSER_REPLACE;
                _content = "";
                continue;
            }
            else if (str::contains(lines[i], "<<<CREATE>>>"))
            {
                _state = PARSER_CREATE;
                _content = "";
                continue;
            }
            else if (str::contains(lines[i], "<<<APPEND>>>"))
            {
                _state = PARSER_APPEND;
                _content = "";
                continue;
            }
            else if (str::contains(lines[i], "<<<NOTE>>>"))
            {
                _state = PARSER_NOTE;
                _note = "";
                continue;
            }
            else if (str::contains(lines[i], "<<<END>>>"))
            {
                try
                {
                    if (_state == PARSER_SEARCH || _state == PARSER_REPLACE)
                    {
                        if (_file_data.empty())
                            error += "Can't search and replace in empty file " + _file + "\n\n";
                        else 
                        {
                            auto data = str::replace(_file_data, _search, _content);
                            if (data == _file_data)
                                error += "No search found in file " + _file + "\n\nSearched : " + _search + "\n\nSould have been replaced with : " + _content + "\n\n";
                            else 
                                files::write(root + files::sep() + _file, data);
                        }
                    }
                    else if (_state == PARSER_CREATE)
                        files::write(root + files::sep() + _file, _content);
                    else if (_state == PARSER_APPEND)
                        files::append(root + files::sep() + _file, _content);
                    else if (_state == PARSER_NOTE)
                        _notes.push_back(_note);
                }
                catch(const std::exception& e)
                {
                    error += "An error happened managning the file " + _file + " :\n" + std::string(e.what()) + "\n\n";
                }

                _actions.push_back(this->action(_state, _file, _search, _content, _note));

                _state = PARSER_NONE;
                _file = "";
                _search = "";
                _content = "";
                _note = "";
                continue;
            }

            if (_state == PARSER_SEARCH)
            {
                if (_search.empty())
                    _search += lines[i];
                else 
                    _search += "\n" + lines[i];
                continue;
            }
            else if (_state == PARSER_REPLACE || _state == PARSER_CREATE || _state == PARSER_APPEND)
            {
                if (_content.empty())
                    _content += lines[i];
                else 
                    _content += "\n" + lines[i];
                continue;
            }
            else if (_state == PARSER_NOTE)
            {
                if (_note.empty())
                    _note += lines[i];
                else 
                    _note += "\n" + lines[i];
                continue;
            }
        }

        if (error.empty())
            return ml::ret::ok();
        else 
            return ml::ret::fail(error);
    }

}
