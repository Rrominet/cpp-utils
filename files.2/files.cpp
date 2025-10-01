#include "./files.h"
#include <filesystem>

#ifdef __linux__
extern "C"
{
#include <sys/file.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
}

#include <libgen.h>         // dirname
#include <unistd.h>         // readlink
#include <linux/limits.h>   // PATH_MAX
#endif

#ifdef _WIN32
#include <windows.h>
#pragma warning(disable : 4996) // need it to exec fopen on Window
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cerrno> 
#include <fstream>
#include "../str.h"

#include <boost/filesystem.hpp>
#include "../mlMath.h"
#include "../Perfs.h"

namespace fs = std::filesystem;

namespace files
{
    std::vector<std::string> _video_extensions = {
        "mp4",   // MPEG-4
        "m4v",   // MPEG-4 Apple variant
        "mov",   // QuickTime
        "avi",   // Audio Video Interleave
        "mkv",   // Matroska
        "webm",  // WebM (VPx + Opus/Vorbis)
        "flv",   // Flash Video
        "f4v",   // Flash MP4 variant
        "wmv",   // Windows Media Video
        "asf",   // Advanced Systems Format
        "mpeg",  // MPEG-1
        "mpg",   // MPEG-1/2
        "mpe",   // MPEG Elementary
        "ts",    // MPEG-TS (Transport Stream)
        "m2ts",  // Blu-ray Transport Stream
        "mts",   // AVCHD
        "3gp",   // 3GPP (mobile)
        "3g2",   // 3GPP2
        "ogv",   // Ogg Video
        "rm",    // RealMedia
        "rmvb",  // RealMedia Variable Bitrate
        "vob",   // DVD Video Object
        "amv",   // Anime Music Video (low-res format)
        "divx",  // DivX container
        "xvid",  // Xvid container variant
        "yuv",   // Raw YUV video
        "mpv",   // MPEG Elementary video
        "bik",   // Bink Video (games)
        "drc",   // Dirac Video
        "mxf",   // Material eXchange Format (broadcast)
        "nut"    // NUT (experimental container)
    };

    std::vector<std::string> _audio_extensions = {
        "mp3",    // MPEG Layer 3
        "aac",    // Advanced Audio Coding
        "m4a",    // MPEG-4 Audio
        "wav",    // Waveform Audio File Format
        "flac",   // Free Lossless Audio Codec
        "ogg",    // Ogg container (often Vorbis or Opus)
        "oga",    // Ogg Audio (raw audio)
        "opus",   // Opus codec
        "alac",   // Apple Lossless
        "aiff",   // Audio Interchange File Format
        "aif",    // AIFF short
        "wma",    // Windows Media Audio
        "amr",    // Adaptive Multi-Rate (used in phones)
        "ac3",    // Dolby Digital
        "eac3",   // Enhanced AC-3 (Dolby Digital Plus)
        "ra",     // RealAudio
        "ram",    // RealAudio Metadata
        "mka",    // Matroska Audio
        "dts",    // Digital Theater Systems audio
        "voc",    // Creative Voice
        "au",     // Sun/NeXT audio format
        "snd",    // Raw Sound data
        "mid",    // MIDI
        "midi",   // MIDI
        "caf"     // Apple Core Audio Format
    };

    std::vector<std::string> _image_extensions = {
        "jpg",    // JPEG
        "jpeg",   // JPEG
        "png",    // Portable Network Graphics
        "bmp",    // Bitmap
        "gif",    // Graphics Interchange Format
        "webp",   // Web Picture format
        "tiff",   // Tagged Image File Format
        "tif",    // TIFF short
        "svg",    // Scalable Vector Graphics
        "ico",    // Icon file
        "heif",   // High Efficiency Image Format
        "heic",   // High Efficiency Image Container (Apple)
        "jp2",    // JPEG 2000
        "dds",    // DirectDraw Surface (games)
        "exr",    // OpenEXR (HDR imaging)
        "raw",    // Raw image data
        "arw",    // Sony RAW
        "cr2",    // Canon RAW
        "nef",    // Nikon RAW
        "orf",    // Olympus RAW
        "rw2",    // Panasonic RAW
        "psd"     // Photoshop Document
    };

    std::vector<std::string> _text_extensions = {
        // Plain text
        "txt", "md", "rst", "log", "csv", "tsv", "nfo", "ini", "conf", "cfg",

        // Programming code
        "c", "cpp", "cc", "cxx", "h", "hpp", "hh", "hxx",
        "py", "java", "js", "ts", "rb", "go", "rs", "php", "swift", "cs", "kt",
        "sh", "bash", "zsh", "fish", "bat", "ps1", "clj", "lua", "pl", "r", "m",

        // Web / Markup
        "html", "htm", "css", "scss", "sass", "xml", "xhtml",

        // Data / Config formats
        "json", "yaml", "yml", "toml", "ini", "properties", "env",

        // Documentation / Markup
        "tex", "latex", "org", "asciidoc", "adoc", "pod",

        // Scripting / Build / Devops
        "Dockerfile", "gitignore", "gitattributes",
        "toml", "gradle",

        // Misc text-ish
        "license", "readme", "todo", "changelog"
    };

    std::string _execDir;
    std::string _execPath;
    std::string _execName;

    void _computeExecPath()
    {
#ifdef __linux__
        char result[PATH_MAX] = {0};
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX -1);
        const char *execpath;
        _execPath = std::string(result);
        if (count != -1)
        {
            result[count] = '\0'; // this is fucked up ! (because readlink don't add the \0 at the end and so there one character that is garbage.)
            execpath = dirname(result);
        }

        _execDir = std::string(result);
#endif

#ifdef _WIN32
        char result [MAX_PATH];
        GetModuleFileNameA(NULL, result, sizeof(result));
        lg2("Win path of the exe", result);

        _execDir = files::parent(std::string(result));
        _execPath = std::string(result);
#endif
        _execName = files::name(_execPath);
    }

    std::string execPath()
    {
        if (!_execPath.empty())
            return _execPath;

        _computeExecPath();
        return _execPath;
    }

    std::string parent(const std::string& path)
    {
        auto splits = str::split(path, sep());
        if (splits.empty())
            return "";
        if (splits.back().empty())
            splits.pop_back();
        splits.pop_back();
#ifdef _WIN32
        std::string gpath;
#else 
        std::string gpath = sep();
#endif

        for (int i=0; i<splits.size(); i++)
        {
            if (splits[i].empty())
                continue;
            if (i == splits.size() - 1)
            {
                gpath += splits[i];
                break;
            }
            gpath += splits[i] + sep();
        }

        return gpath;
    }

    std::string execDir()
    {
        if (!_execDir.empty())
            return _execDir;

        _computeExecPath();
        return _execDir;
    }

    bool exists(const std::string& path)
    {
        try
        {
            bool _r = fs::exists(fs::path(path));
            if (_r)
                return _r;
            else if (files::isSymbolic(path))
                return true;
            return false;
        }
        catch(...)
        {
            return false;
        }
    }

    bool isSymbolic(const std::string& path)
    {
        return fs::is_symlink(path);
    }

    bool mkdir(const std::string& path)
    {
        try
        {
            fs::create_directories(path);
            return true;
        }
        catch(const std::exception& e)
        {
            return false;
        }
    }

    std::string realPath(const std::string& path)
    {
        if (!files::isSymbolic(path))
            return path;
        return fs::read_symlink(path);
    }

    bool isFile(const std::string& path)
    {
        try
        {
            return fs::is_regular_file(files::realPath(path));
        }
        catch(const std::exception& e)
        {
            return false;
        }
    }

    bool isDir(const std::string& path)
    {
        try
        {
            return fs::is_directory(files::realPath(path));
        }
        catch(const std::exception& e)
        {
            return false;
        }
    }

    bool isHidden(const std::string& path)
    {
        if (files::name(path).at(0) == '.')
            return true;
        return false;
    }

    std::string name(const std::string& path)
    {
        if (path.empty())
            return path;

        if (!str::contains(path, sep()))
            return path;

        std::string gpath = path;
        if (str::last(path) == sep()[0])
            gpath = path.substr(0, path.size() - 1);

        std::vector<std::string> tmp = str::split(gpath, sep());
        return tmp.back();
    }

    std::string baseName(const std::string& path)
    {
        std::string _r = files::name(path); 
        size_t found = _r.rfind("."); 
        if (found == std::string::npos)
            return _r; 
        return _r.substr(0, found);
    }

    std::string ext(const std::string& path)
    {
        auto _r = files::name(path); 
        size_t found = _r.rfind("."); 
        if (found == std::string::npos)
            return ""; 
        if (_r.size() - 1 == found)
            return ""; 
        return str::lower(_r.substr(found + 1));
    }

    std::string tmp()
    {
        return fs::temp_directory_path().string();
    }

    std::string sep()
    {
#ifdef _WIN32
        return "\\";
#else 
        return "/";
#endif
    }

    std::string execName()
    {
        if (!_execName.empty())
            return _execName;

        _computeExecPath();
        return _execName;
    }

    int lock(const std::string& path, bool shared )
    {
        std::string content;
        auto fd = open(path.c_str(), O_RDONLY, 0666);
        if (fd < 0)
            throw std::runtime_error("Unable to open file " + path);

        int res = 0;
        if (shared)
            res = flock(fd, LOCK_SH);
        else 
            res = flock(fd, LOCK_EX);
        if (res < 0)
        {
            close(fd);
            throw std::runtime_error("Unable to lock file " + path);
        }

        lg("file " + path + " locked.");
        return fd;
    }

    int exlock(const std::string& path){return files::lock(path, false);}

    void unlock(int fd)
    {
        int res = flock(fd, LOCK_UN);
        if (res < 0)
        {
            close(fd);
            throw std::runtime_error("Unable to unlock filedescriptor " + std::to_string(fd));
        }

        lg("filedescriptor " + std::to_string(fd) + " unlocked.");
        close(fd);
    }

    std::string read(const std::string& path)
    {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0)
            throw std::runtime_error("Unable to open file " + path + " to read it.");

        std::string content;
        auto size = files::size(path);
        content.resize(size);
        if (::read(fd, &content[0], size) < 0)
        {
            close(fd);
            throw std::runtime_error("Unable to read file " + path);
        }

        close(fd);
        return content;
    }

    std::string content(const std::string& path){return files::read(path);}

    size_t write(const std::string& path, const std::string& content, int flags, int permissions)
    {
        int fd = open(path.c_str(), flags , permissions);
        if (fd < 0)
        {
            int errnum = errno;
            throw std::runtime_error("Unable to open file " + path + " to write in it : " + std::strerror(errnum));
        }
        // If the file is not opened in append mode, truncate it
        if (!(flags & O_APPEND)) 
        {
            if (ftruncate(fd, 0) < 0) {
                close(fd);
                throw std::runtime_error("Unable to truncate file " + path + ": " + std::strerror(errno));
            }
        }

        auto written = ::write(fd, content.c_str(), content.size());
        if (written < 0)
        {
            close(fd);
            throw std::runtime_error("Unable to write in file " + path);
        }

        close(fd);

        return written;
    }

    size_t write(const std::string& path, const std::string& content, int permissions){return files::write(path, content, O_WRONLY | O_CREAT, permissions);}
    size_t append(const std::string& path, const std::string& content){return files::write(path, content, O_WRONLY | O_APPEND | O_CREAT, 0666);}

    size_t size(const std::string& path)
    {
        if (files::isSymbolic(path))
            return 0;

        if (files::isDir(path))
            throw std::runtime_error("files::size(), " + path + " is a directory. Use the function files::size_pgr() instead.");
        try {
            return fs::file_size(path); 
        }
        catch (const std::exception&) {
            return 0;
        }
    }

    std::string readable_size(size_t size, int round)
    {
        long double dsize = size;
        std::string _r = "";
        std::vector<std::string> units{"o", "Ko", "Mo", "Go", "To", "Po", "Eo", "Zo"};

        for (auto u : units)
        {
            if (abs(dsize)<1024.0)
            {
                _r = "";
                _r += math::round(dsize) + " " + u;
                return _r;
            }
            dsize /= 1024.0;
        }
        _r = "";
        _r += math::round(dsize) + " Yo";
        return _r;
    }

    std::vector<std::string> ls(const std::string& path, SortType sort, bool onlyFilename)
    {
        std::vector<std::string> files;
        for (const auto& entry : fs::directory_iterator(path, fs::directory_options::follow_directory_symlink))
        {
            if (onlyFilename)
                files.push_back(files::name(entry.path().string()));
            else
                files.push_back(entry.path().string());
        }
        switch (sort)
        {
            case NAME:
                std::sort(files.begin(), files.end(), files::sortByName);
                break;
            case NAME_INV:
                std::sort(files.begin(), files.end(), files::sortByNameInv);
                break;
            case NONE:
                break;
        }
        return files;
    }

    std::vector<std::string> ls_reccursive(const std::string& path, SortType sort , bool onlyFilename)
    {
        std::vector<std::string> files;
        auto iterator = fs::recursive_directory_iterator(path, fs::directory_options::follow_directory_symlink);
        for (const auto& entry : iterator)
        {
            if (onlyFilename)
                files.push_back(files::name(entry.path().string()));
            else
                files.push_back(entry.path().string());
        }
        switch (sort)
        {
            case NAME:
                std::sort(files.begin(), files.end(), files::sortByName);
                break;
            case NAME_INV:
                std::sort(files.begin(), files.end(), files::sortByNameInv);
                break;
            case NONE:
                break;
        }
        return files;
    }

    bool sortByName(const std::string& a, const std::string& b)
    {
        return files::baseName(a) < files::baseName(b);
    }
    bool sortByNameInv(const std::string& a, const std::string& b)
    {
        return files::baseName(a) > files::baseName(b);
    }

    // return false if the file doesn't exist
    // throw if there was another error
    // return true if the file was removed
    bool remove(const std::string& path)
    {
        if (files::isDir(path))
            return fs::remove_all(path);
        return fs::remove(path);
    }

    void copy(const std::string& from, const std::string& to)
    {
        const auto options = fs::copy_options::update_existing | fs::copy_options::recursive;
        fs::copy(from, to, options);
    }

    void copy(const std::vector<std::string>& files, const std::string& inDir)
    {
        if (!files::exists(inDir))
            files::mkdir(inDir);
        if (!files::isDir(inDir))
            throw std::runtime_error(inDir + " is not a directory.");
        for (const auto& file : files)
            copy(file, inDir + files::sep() + files::name(file));
    }

    // it does NOT check file existence and file type, but you NEED to check it yourself before callign this function (for performance reasons)
    // same, to should be writable.
    // Will crash if from is symlink should be check before.
    unsigned long _copy_file_pgr(const std::string& from, const std::string& to,
            const std::function<void (CopyProgress&)>& progress, unsigned int pgrInterval, CopyProgress& track)
    {
        track.copying = from;
        FILE* src = fopen(from.c_str(), "rb");
        FILE* dst = fopen(to.c_str(), "wb");

        unsigned char buff[8192];
        size_t n, m;
        Perfs perfs;
        perfs.start();
        do {
            n = fread(buff, 1, sizeof(buff), src);
            if (n) 
            {
                m = fwrite(buff, 1, n, dst);
                track.copied += m;
                if (perfs.hasPassed(pgrInterval))
                {                 
                    progress(track);
                    perfs.start();
                }
            }
            else
                m = 0;
        }while((n>0) && (n == m));

        fclose(src);
        fclose(dst);
        track.files_copied++;
        progress(track);
        return track.copied;
    }

    void copy_pgr(const std::string& from, const std::string& to,
            const std::function<void (CopyProgress&)>& progress, CopyProgress& track, 
            const CopyRules& rules,
            unsigned int pgrInterval)
    {
        if (!files::exists(from))
            return;

        if (files::exists(to) && !files::isDir(to))
        {
            if (rules.overwrite == "no")
                return;
            else if (rules.overwrite == "size")
            {
                if (files::size(from) == files::size(to))
                    return;
            }
        }

        if (!files::isDir(files::parent(to)))
        {
            auto succ = files::mkdir(files::parent(to));
            if (!succ)
                return;
        }

        if (files::isDir(from))
        {
            for (const auto& file : files::ls(from))
                copy_pgr(file, to + files::sep() + files::name(file), progress, track, rules, pgrInterval);
        }

        else if (files::isSymbolic(from))
        {
            auto sym = files::target(from);
            files::createLink(sym, to);
            track.files_copied ++;
            progress(track);
            return;
        }

        else 
            _copy_file_pgr(from, to, progress, pgrInterval, track);
    }

    void copy_pgr(const std::vector<std::string>& files, const std::string& inDir,
            const std::function<void (CopyProgress&)>& progress, CopyProgress& track, 
            const CopyRules& rules,
            unsigned int pgrInterval)
    {
        for (const auto& file : files)
            copy_pgr(file, inDir + files::sep() + files::name(file), progress, track, rules, pgrInterval);
    }

    void _size_dir_pgr(const std::string& path, const std::function<void (CopyProgress&)>& progress, Perfs& perfs, unsigned int pgrInterval, CopyProgress& track)
    {
        for (const auto& file : files::ls(path)) 
        {
            if (files::isDir(file) && !files::isSymbolic(file))
                _size_dir_pgr(file, progress, perfs, pgrInterval, track);
            else if (files::isSymbolic(file))
            {}
            else
            {
                auto fsize = files::size(file);
                track.copied += fsize;
                if (perfs.hasPassed(pgrInterval))
                {
                    progress(track);
                    perfs.start();
                }
            }
        }
    }

    unsigned long size_pgr(const std::string& path,
            const std::function<void (CopyProgress&)>& progress, CopyProgress& track, unsigned int pgrInterval)
    {
        if (!files::exists(path))
        {
            progress(track);
            return track.copied;
        }

        if (files::isFile(path))
        {
            track.copied = files::size(path);
            progress(track);
            return track.copied;
        }

        if (files::isDir(path))
        {
            Perfs perfs;
            perfs.start();
            files::_size_dir_pgr(path, progress, perfs, pgrInterval, track);
            progress(track);
            return track.copied;
        }

        return 0;
    }

    unsigned long size_pgr(const std::vector<std::string>& files,
            const std::function<void (CopyProgress&)>& progress, CopyProgress& track, unsigned int pgrInterval)
    {
        for (const auto& file : files)
            size_pgr(file, progress, track, pgrInterval);

        return track.copied;
    }

    void move(const std::string& from, const std::string& to)
    {
        fs::rename(from, to);
    }

    void move(const std::vector<std::string>& files, const std::string& inDir)
    {
        for (const auto& file : files)
            move(file, inDir + files::sep() + files::name(file));
    }

    std::string cleaned(std::string path)
    {
#ifdef __linux__
        if (path[0] != '/')
            path = files::sep() + path;

        path = str::replace(path, "\\", "/");
#endif
#ifdef __EMSCRIPTEN__
        path = str::replace(path, "\\", "/");
#endif 
#ifdef _WIN32
        path = str::replace(path, "/", "\\");
        if (path.size() > 1 && path[1] != ':')
        {
            path = path.substr(1);
            path = "C:\\" + path;
        }
#endif
        path = str::replace(path, files::sep() + files::sep(), files::sep());
        return path;
    }

    int64_t lastTimeModified(const std::string& path)
    {
        try
        {
            boost::filesystem::path file_path(path);
            std::time_t last_modified = boost::filesystem::last_write_time(file_path);
            return static_cast<int64_t>(last_modified);
        }
        catch(...)
        {
            return 0;
        }
    }

    std::vector<unsigned char> bContent(const std::string& path)
    {
        if (!files::exists(path))
            return {};
        std::ifstream input(path, std::ios::binary);
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

        return buffer;
    }

    std::string home()
    {
        char const* val = std::getenv("HOME"); 
        return val == NULL ? std::string() : std::string(val);
    }

    std::string configPath()
    {
        return files::home() + files::sep() + ".config";
    }

    bool isImg(const std::string& path)
    {
        std::vector<std::string> extensions {
            "jpg", "jpeg", "jpe", "jif", "jfif", "jfi", "gif", "png", "tif", "tiff", "webp", "psd", "raw", "cr2", "arw", "nrw", "k25", "bmp", "dib", "ico", "heif", "heic", "ind", "indd", "indt", "jp2", "j2k", "jpf", "jpx", "jpm", "mj2"
        };
        return vc::contains(extensions, files::ext(path));
    }

    std::string target(const std::string& symlink)
    {
        return fs::read_symlink(symlink);
    }

    bool createLink(const std::string& target, const std::string& path)
    {
        try
        {
            fs::create_symlink(target, path);
        }catch(...){return false;}
        return true;
    }

    bool isAbsolute(const std::string& path)
    {
        return path[0] == '/' || path[1] == ':';
    }

    std::string absolutePathToTarget(const std::string& linkpath)
    {
        auto symlink = files::target(linkpath);
        if (files::isAbsolute(symlink))
            return symlink;
        return files::parent(linkpath) + files::sep() + symlink;
    }

    std::string cwd()
    {
        return std::filesystem::current_path().string();
    }

    FileType type(const std::string& path)
    {
        auto ext = files::ext(path);
        if (vc::contains(_text_extensions, ext))
            return FileType::TEXT;
        if (vc::contains(_video_extensions, ext))
            return FileType::VIDEO;
        if (vc::contains(_audio_extensions, ext))
            return FileType::SOUND;
        if (vc::contains(_image_extensions, ext))
            return FileType::IMAGE;

        return FileType::UNKNOWN;
    }
}
