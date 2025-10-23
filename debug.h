#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <string>


// this changed
#ifdef mydebug
#define lg(x) std::cerr <<__FILE__ << "("<< __LINE__ << ") -- " << x << std::endl
#define lg2(y, x) std::cerr <<__FILE__ << "("<< __LINE__ << ") -- " << y << " : " << x << std::endl
#define db_write(x) std::cerr <<__FILE__ << "("<< __LINE__ << ") -- " << x << std::endl;db::write(x)

#define db_write2(x, y)\
    std::cerr <<__FILE__ << "("<< __LINE__ << ") -- " << x << " : " << y << std::endl;\
{\
    std::ostringstream ss; ss << x << " : " <<y << std::endl;\
        db::write(ss.str());\
}
#else 
#define lg(x)
#define lg2(y, x)
#ifdef NO_LOG
#define db_write(x)
#define db_write2(x, y)
#else
#define db_write(x) db::write(x)
#define db_write2(x, y) {\
    std::ostringstream ss; ss << x << " : " << y << std::endl;\
        db::write(ss.str());\
}
#endif

#endif

namespace db
{
    void log(const std::vector<std::string> &vec);

    template <typename T>
        static void log(const std::vector<T> &vec)
        {
            for (auto &c : vec)
                lg(c);             
        }

    void write(std::string tolog);
    void setLogFile(const std::string& path);
    void setLogFile(const std::string& dirpath, char* programname);
}
