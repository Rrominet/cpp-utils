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
        auto issues_s = str::split(outData, "\n\n---\n\n");
        _issues.clear();
        for (const auto& issue_s : issues_s)
        {
            auto issue_r = DebuggerAgent::fromString(issue_s);
            if (issue_r.success)
                _issues.push_back(issue_r.value);
        }

        return outData;
    }

    ml::Ret<DebuggerAgent::Issue> DebuggerAgent::fromString(const std::string& s)
    {

        const auto header_end = s.find('\n');
        if (header_end == std::string::npos)
            return ml::ret::fail<Issue>("Invalid issue, no \n in header");

        const auto header = s.substr(0, header_end);
        if (header.size() < 3 || header[0] != '[')
            return ml::ret::fail<Issue>("Invalid issue, no [in header");

        const auto type_end = header.find(']');
        if (type_end == std::string::npos)
            return ml::ret::fail<Issue>("Invalid issue, no ] in header");

        const auto type = header.substr(1, type_end - 1);
        Issue i;

        if (type == "CRITICAL")
            i.type = "critical";
        else if (type == "HIGH")
            i.type = "high";
        else if (type == "MEDIUM")
            i.type = "medium";
        else if (type == "LOW")
            i.type = "low";
        else 
            i.type = "low";

        i.content = s.substr(header_end + 1);
        return ml::ret::ok<Issue>(i);
    }
}
