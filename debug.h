#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <string>


namespace db
{
    void log(const std::vector<std::string> &vec);

//useful for thread sync logs
    void log_sync(const std::string& str);

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

#ifdef mydebug
#define lg(x) do { std::ostringstream ss; ss << __FILE__ << "(" << __LINE__ << ") -- " << x; db::log_sync(ss.str()); } while(0)
#define lg2(y, x) do { std::ostringstream ss; ss << __FILE__ << "(" << __LINE__ << ") -- " << y << " : " << x; db::log_sync(ss.str()); } while(0)
#define db_write(x) do { std::ostringstream ss; ss << __FILE__ << "(" << __LINE__ << ") -- " << x; db::log_sync(ss.str()); db::write(ss.str()); } while(0)

#define db_write2(x, y)\
do {\
    std::ostringstream ss; ss << __FILE__ << "(" << __LINE__ << ") -- " << x << " : " << y;\
    db::log_sync(ss.str());\
    std::ostringstream ss2; ss2 << x << " : " << y << std::endl;\
    db::write(ss2.str());\
} while(0)
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

