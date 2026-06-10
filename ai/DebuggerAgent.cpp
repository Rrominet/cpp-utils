#include "./DebuggerAgent.h"
#include "../str.h"

namespace ml
{
    DebuggerAgent::DebuggerAgent(const std::string& name) : Agent("DebuggerAgent", name)
    {
        this->setContext(_generateCtx());
        this->addAfterLlmCall([](Agent* self, const std::string& inData, const std::string& outData) -> std::string {
            return static_cast<DebuggerAgent*>(self)->processOutput(inData, outData);
        });
    }

    std::string DebuggerAgent::_generateCtx()
    {
        return R"(You are a code auditor. Brutal, direct, no bullshit.
Your job is to analyze code and find every bug, potential issue and design problem.

Rules:
- Order all findings by urgency: Critical → High → Medium → Low
- No praise, no intro, no conclusion, no fluff
- Respect and enforce KISS — over-engineered code is a bug
- If something will definitely break, say it will definitely break
- If something is risky or ugly, say exactly why

For each issue use this format:

[CRITICAL/HIGH/MEDIUM/LOW] filename — line X
Problem: ...
Why it matters: ...
Fix: ...

You separate each issur with the separator \n\n---\n\n
If not your response, would be unparsable and useless.

When the user sends code, go straight to the list. Nothing else.)";
    }

    std::string DebuggerAgent::processOutput(const std::string& inData, const std::string& outData)
    {
        _issues = str::split(outData, "\n\n---\n\n");
        return outData;
    }
}
