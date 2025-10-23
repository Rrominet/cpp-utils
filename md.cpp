#include "./md.h"

namespace md
{
    std::string bloc_type(MD_BLOCKTYPE type)
    {
        switch(type) {
            case MD_BLOCK_DOC:
                return "body";
            case MD_BLOCK_QUOTE:
                return "blockquote";
            case MD_BLOCK_UL:
                return "ul";
            case MD_BLOCK_OL:
                return "ol";
            case MD_BLOCK_LI:
                return "li";
            case MD_BLOCK_HR:
                return "hr";
            case MD_BLOCK_H:
                return "h";
            case MD_BLOCK_CODE:
                return "code-block";
            case MD_BLOCK_HTML:
                return "raw-html";
            case MD_BLOCK_P:
                return "p";
            case MD_BLOCK_TABLE:
                return "table";
            case MD_BLOCK_THEAD:
                return "thead";
            case MD_BLOCK_TBODY:
                return "tbody";
            case MD_BLOCK_TR:
                return "tr";
            case MD_BLOCK_TH:
                return "th";
            case MD_BLOCK_TD:
                return "td";
                break;
        }
    }

    std::string span_type(MD_SPANTYPE type)
    {
        switch(type) {

            case MD_SPAN_EM:
                return "em";
            case MD_SPAN_STRONG:
                return "b";
            case MD_SPAN_A:
                return "a";
            case MD_SPAN_IMG:
                return "img";
            case MD_SPAN_CODE:
                return "code";
            case MD_SPAN_DEL:
                return "del";
            case MD_SPAN_LATEXMATH:
                return "latexmath";
            case MD_SPAN_LATEXMATH_DISPLAY:
                return "latexmath-disp";
            case MD_SPAN_WIKILINK:
                return "wikilink";
            case MD_SPAN_U:
                return "i";
                break;
        }
    }

    std::function<void(MD_BLOCKTYPE, void*, void*)> _on_enter_block;
    std::function<void(MD_BLOCKTYPE, void*, void*)> _on_leave_block;
    std::function<void(MD_SPANTYPE, void*, void*)> _on_enter_span;
    std::function<void(MD_SPANTYPE, void*, void*)> _on_leave_span;
    std::function<void(const std::string&, void*)> _on_text;

    static int exec_on_enter_block(MD_BLOCKTYPE type, void* details, void *userdata) 
    {
        _on_enter_block(type, details, userdata);
        return 0;    
    }

    static int exec_on_leave_block(MD_BLOCKTYPE type, void* details, void *userdata) 
    {
        _on_leave_block(type, details, userdata);
        return 0;    
    }

    static int exec_on_enter_span(MD_SPANTYPE type, void* details, void *userdata) 
    {
        _on_enter_span(type, details, userdata);
        return 0;    
    }

    static int exec_on_leave_span(MD_SPANTYPE type, void* details, void *userdata) 
    {
        _on_leave_span(type, details, userdata);
        return 0;    
    }

    static int exec_on_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
    {
        _on_text(std::string(text, size), userdata);
        return 0;
    }

    int parse(const std::string& to_parse, 
            const std::function<void(MD_BLOCKTYPE, void*, void*)> &on_enter_block,
            const std::function<void(MD_BLOCKTYPE, void*, void*)> &on_leave_block,
            const std::function<void(MD_SPANTYPE, void*, void*)> &on_enter_span,
            const std::function<void(MD_SPANTYPE, void*, void*)> &on_leave_span,
            const std::function<void(const std::string&, void*)> &on_text,
            int flags,
            void* userdata
            )
    {
        
        _on_enter_block = on_enter_block;
        _on_leave_block = on_leave_block;
        _on_enter_span = on_enter_span;
        _on_leave_span = on_leave_span;
        _on_text = on_text;

        MD_PARSER parser; 
        parser.abi_version = 0;
        parser.flags = flags;
        parser.enter_block = exec_on_enter_block;
        parser.leave_block = exec_on_leave_block;
        parser.enter_span = exec_on_enter_span;
        parser.leave_span = exec_on_leave_span;
        parser.text = exec_on_text;

        return md_parse(to_parse.c_str(), to_parse.size(), &parser, userdata);
    }
}

