#pragma once
#include <exception>
#include <string>

namespace error
{
    class base: public std::exception
    {
        private : 
            std::string _msg;
        public: 
            base(const std::string& reason) : std::exception(), _msg(reason){}
            const char * what() const noexcept override {
                return _msg.c_str();
            }
    };

    class file: public base{ using base::base;};
    class missing_json_attr: public base{ using base::base;};
    class missing_attr: public base{ using base::base;};
    class missing_arg: public base{ using base::base;};
    class bad_attr: public base{ using base::base;};
    class bad_arg: public base{ using base::base;};
    class not_found: public base{ using base::base;};
    class file_not_found: public base{ using base::base;};
    class dir_not_found: public base{ using base::base;};
    class image_not_loaded: public base{ using base::base;};
    class size: public base{ using base::base;};
    class bad_type: public base{ using base::base;};
}
