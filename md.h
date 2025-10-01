#pragma once
extern "C"
{
#include "./md4c/md4c.h"
}
#include <string>
#include <functional>
//a binding to the md4c library (see foldef ./md4c) in cpp
//

namespace md
{
    std::string bloc_type(MD_BLOCKTYPE type);
    std::string span_type(MD_SPANTYPE type);
    //0 is returned on success
    int parse(const std::string& to_parse, 
            const std::function<void(MD_BLOCKTYPE, void*, void*)> &on_enter_block,
            const std::function<void(MD_BLOCKTYPE, void*, void*)> &on_leave_block,
            const std::function<void(MD_SPANTYPE, void*, void*)> &on_enter_span,
            const std::function<void(MD_SPANTYPE, void*, void*)> &on_leave_span,
            const std::function<void(const std::string&, void*)> &on_text,
            int flags =  MD_FLAG_WIKILINKS | MD_FLAG_TABLES | MD_FLAG_UNDERLINE | MD_FLAG_TASKLISTS, 
            void* userdata = nullptr
            );
}
