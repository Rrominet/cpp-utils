#include "./utils.h"
#include "../files.2/files.h"
#include "../debug.h"
#include "../str.h"

namespace ml
{
    namespace ai
    {
        std::string filesForLLM(const std::vector<std::string>& files, const std::string& root)
        {
            std::string _r;	
            for (auto& f : files)
                _r += fileForLLM(f, root);
            return _r;
        }

        std::string filepathRelative(const std::string& filepath, const std::string& root)
        {
            if (root.empty())
                return filepath;
            auto _r = str::replace(filepath, root, "");	
            try
            {
                if (_r.at(0) == '/' && !root.empty())
                    _r = _r.substr(1);
            }
            catch(const std::exception& e)
            {
                lg(e.what());
            }
            return _r;
        }

        std::string fileForLLM(const std::string& filepath, const std::string& root)
        {
            try
            {
                if(files::name(filepath).at(0) == '.')
                    return "";
            }
            catch(const std::exception& e){
                return "";
            }

            std::string _r;	
            _r += filepathRelative(filepath, root) + ":\n";
            if (files::isDir(filepath))
                _r += "is a directory.\n";
            else
            {
                try
                {
                    _r += files::read(filepath) + "\n";
                }
                catch(const std::exception& e)
                {
                    lg("Error reading file " + filepath + " : " + std::string(e.what()));
                }
            }
            _r += "================\n\n";
            return _r;
        }
    }
}
