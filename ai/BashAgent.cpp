#include "./BashAgent.h"
#include "../mlprocess.h"
#include "../str.h"
#include "../files.2/files.h"

namespace ml
{
    BashAgent::BashAgent(const std::string& name) : Agent("BashAgent", name)
    {
        this->setContext(_generateCtx());
        this->addBeforeLlmCall([](Agent* self, const std::string& inData) -> std::string{static_cast<BashAgent*>(self)->checkBashPath(); return "";});
        this->addAfterLlmCall([](Agent* self, const std::string& inData, const std::string& outData) -> std::string{return static_cast<BashAgent*>(self)->execBash(inData, outData);});
    }

    std::string BashAgent::_generateCtx()
    {
        return "You respond me with bash commands that execute the task I want to perform.\nDo not respond ANYTHING ELSE than bash code that can be executed on LINUX. If you add anything else in your response, it will be unparsable and unusable."	;
    }

    std::string BashAgent::execBash(const std::string& inData,const std::string& outData)
    {
        std::string bash = outData; 
        bash = str::replace(bash, "```bash\n", "");
        bash = str::replace(bash, "\n```", "");
        lg("bash command : " << bash);

        auto execPath = bashPath();
        if (execPath.empty())
        {
            this->setError("Could not find bash on the system (after llm call, really wierd.)");
            return "";
        }

        ml::Vec<std::string> fullcmd = {execPath, "-c", bash };
        auto pout = process::exec2(fullcmd);
        if (pout.exitCode != 0)
            this->setError("The exit code is not 0 : " + std::to_string(pout.exitCode) + "\nMore infos : " + pout.stdout + "\n\n" + pout.stderr);
        return pout.stdout;
    }

    std::string BashAgent::bashPath()
    {
        if (files::exists("/usr/bin/bash")) 
            return "/usr/bin/bash";
        if (files::exists("/bin/bash")) 
            return "/bin/bash";
        if (files::exists("/usr/local/bin/bash"))
            return "/usr/local/bin/bash";
        return "";
    }

    void BashAgent::checkBashPath()
    {
        auto test = bashPath();        
        if (test.empty())
            this->setError("Could not find bash on the system");
    }
}
