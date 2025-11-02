#pragma once
#include <boost/version.hpp>

#if BOOST_VERSION >= 108400
#include <boost/filesystem/detail/path_traits.hpp>
#else 
#include <boost/filesystem/path_traits.hpp>
#endif

#include <string>
#include <vector>

#define _fc +files::sep()+

namespace files
{
    enum SortType
    {
        NONE,
        NAME,
        NAME_INV,
        SIZE,
        SIZE_INV,
        TIME,
        TIME_INV
    };

    enum FileType
    {
        UNKNOWN,
        TEXT,
        IMAGE,
        VIDEO,
        SOUND,
    };

    struct CopyRules
    {
        // no, yes, size
        std::string overwrite = "no";
    };

    std::string execPath();
    std::string parent(const std::string& path);
    std::string name(const std::string& path);
    std::string baseName(const std::string& path);
    std::string ext(const std::string& path);

    //return the same path is it's a regular file/dir
    //return the target dir/file if is symlink
    std::string realPath(const std::string& path);

    std::string execDir();
    bool exists(const std::string& path);
    bool isSymbolic(const std::string& path);
    bool mkdir(const std::string& path);
    bool isFile(const std::string& path);
    bool isDir(const std::string& path);
    bool isHidden(const std::string& path);
    std::string tmp();
    std::string sep();
    std::string execName();
    std::string target(const std::string& symlink);
    bool createLink(const std::string& target, const std::string& path);

    // return true if the path is absolute (start with / or C: or equivalent)
    bool isAbsolute(const std::string& path);
    std::string absolutePathToTarget(const std::string& linkpath);

    int lock(const std::string& path, bool shared = true);
    int exlock(const std::string& path);
    void unlock(int fd);

    // like a multithread system, this function don'et check if the file is lock, you need to lock it before if you need it
    std::string read(const std::string& path);
    // read allias
    std::string content(const std::string& path);
    size_t write(const std::string& path, const std::string& content, int flags, int permissions=0666);
    size_t write(const std::string& path, const std::string& content, int permissions=0666);
    size_t append(const std::string& path, const std::string& content);
    size_t size(const std::string& path);
    std::string readable_size(size_t size, int round=2);

    size_t write(const std::string& path, const std::vector<unsigned char>& content, int flags, int permissions=0666);
    size_t write(const std::string& path, void* content, size_t size, int flags, int permissions=0666);

    size_t write(const std::string& path, const std::vector<unsigned char>& content, int permissions=0666);
    size_t write(const std::string& path, void* content, size_t size, int permissions=0666);

    std::vector<unsigned char> bContent(const std::string& path);

    std::vector<std::string> ls(const std::string& path, SortType sort = NAME, bool onlyFilename=false);
    std::vector<std::string> ls_reccursive(const std::string& path, SortType sort = NAME, bool onlyFilename=false);

    // return false if the file doesn't exist
    // throw if there was another error
    // return true if the file was removed
    bool remove(const std::string& path);

    bool sortByName(const std::string& a, const std::string& b);
    bool sortByNameInv(const std::string& a, const std::string& b);
//     bool sortBySize(const std::string& a, const std::string& b);
//     bool sortBySizeInv(const std::string& a, const std::string& b);
//     bool sortByTime(const std::string& a, const std::string& b);
//     bool sortByTimeInv(const std::string& a, const std::string& b);
    
    // this will overwrite if to exsits
    // it works for dirs too reccursively.
    // TODO : can't track progress here so its faster but less flexible. Create version where the user could track the progress.
    void copy(const std::string& from, const std::string& to);
    void copy(const std::vector<std::string>& files, const std::string& inDir);
    

    struct CopyProgress
    {
        // in bytes
        unsigned long copied = 0;   
        unsigned long files_copied = 0;
        unsigned long files_to_copy = 0;
        std::string copying;
    };

    // TODO : need a way to track error state.
    void copy_pgr(const std::string& from, const std::string& to,
            const std::function<void (CopyProgress&)>& progress,
            CopyProgress& track,
            const CopyRules& rules,
            unsigned int pgrInterval=16);

    void copy_pgr(const std::vector<std::string>& files, const std::string& inDir,
            const std::function<void (CopyProgress&)>& progress,
            CopyProgress& track,
            const CopyRules& rules,
            unsigned int pgrInterval=16);

    unsigned long size_pgr(const std::string& path,
            const std::function<void (CopyProgress&)>& progress, CopyProgress& track,
            unsigned int pgrInterval=16);

    unsigned long size_pgr(const std::vector<std::string>& files,
            const std::function<void (CopyProgress&)>& progress, CopyProgress& track,
            unsigned int pgrInterval=16);

    void move(const std::string& from, const std::string& to);
    void move(const std::vector<std::string>& files, const std::string& inDir);

    std::string cleaned(std::string path);
    int64_t lastTimeModified(const std::string& path);

    std::string home();
    std::string configPath();
    bool isImg(const std::string& path);

    std::string cwd();

    FileType type(const std::string& path);

}
