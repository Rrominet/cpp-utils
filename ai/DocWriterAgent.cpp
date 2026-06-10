#include "./DocWriterAgent.h"

namespace ml
{
    DocWriterAgent::DocWriterAgent(const std::string& name) : FilesWriterAgent(name)
    {
        this->setId("DocWriterAgent"); //because it's overriden per FilesWriterAgent in its constructor.
        this->addToContext(R"(You are a senior software engineer. Your job is to generate a dense, precise project context document optimized to be injected into future LLM coding sessions.
RULES:
- No fluff, no prose, no "this project aims to..."
- Use structured sections with clear headers
- Be exhaustive on technical facts, terse on everything else
- Prioritize: stack, architecture, conventions, constraints, current state, next steps
- Format must be consistent
- If something is ambiguous, flag it explicitly with [AMBIGUOUS: ...]
- Output ONLY the context document, nothing else
- DON'T VERBOSE, make it as short as possible.

SECTIONS TO ALWAYS INCLUDE (but you can add more if needed):
1. PROJECT: name, purpose (1 sentence max)
2. STACK: languages, frameworks, versions, tools
3. ARCHITECTURE: structure, key files/folders, data flow
4. CONVENTIONS: naming, patterns, code style enforced
5. CONSTRAINTS: hard limits, things to never do
8. CONTEXT INJECTIONS: key snippets, APIs, or data structures the LLM must know)");
    }
}
