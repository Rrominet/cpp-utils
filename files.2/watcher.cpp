#include "watcher.h"
#include <unordered_map>
#include <sys/inotify.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include "./files.h"
#include "../debug.h"
#include "../thread.h"

// this is an API that watch the filesystem for changes asynchronously and emit events to the user.
// the watching start automatically when you call at least one time watcher::add()
// its stop automatically if you call watcher::remove() until there is no more path to watch...
// It use the Events API. you can react to the events : 
//  - file-created
//  - file-modified
//  - file-deleted
//  - file-moved-from (file has beed moved from the watched dir, it contains its old filepath infos)
//  - file-moved-to (file has been add the this watched dir (but from a move command, not a copy or a touch one))
//  - moved-self (when the watched path has been moved and so is no longer valid) - when this is called the watched path is removed
//  Doing ml::watcher::events().add("file-created", []{...})...
//
namespace ml
{
    namespace watcher 
    {
        Events _events;
        bool _running = false;
        int _fd = -1;

        std::mutex _watchers_map_mtx;
        std::unordered_map<std::string, int> _watchers;

        Events& events(){return _events;}

        void dummy_handler(int sigum){}

        // you need to call this at the bigining of your program
        void init()
        {
            _fd = inotify_init();
            if (_fd < 0)
            {
                perror("inotify_init");
                throw std::runtime_error("Can't init inotify...");
            }
        }
        
        void start()
        {
            auto f = [&]
            {
                char _buffer[8192];
                while(true)
                {
                    int length = read(_fd, _buffer, sizeof(_buffer));

                    if (length == -1) 
                    {
                        if (errno == EBADF) 
                        {
                            lg("inotify fd closed, stopping watcher thread...");
                            return;
                        }
                    }

                    int i = 0;
                    while(i<length)
                    {
                        struct inotify_event *event = (struct inotify_event *)&_buffer[i];
                        if (event->len) 
                        {
                            std::string root;
                            {
                                LK(_watchers_map_mtx);
                                for (auto& w : _watchers)
                                {
                                    if (w.second == event->wd)
                                    {
                                        root = w.first;
                                        break;
                                    }
                                }
                            }
                            std::string p = root + files::sep() + event->name;
                            if (event->mask & IN_CREATE) 
                                _events.emit("file-created", p);
                            else if (event->mask & IN_DELETE) 
                                _events.emit("file-deleted", p);
                            else if (event->mask & IN_MODIFY) 
                                _events.emit("file-modified", p);
                            else if (event->mask & IN_MOVED_FROM)
                                _events.emit("file-moved-from", p);
                            else if (event->mask & IN_MOVED_TO)
                                _events.emit("file-moved-to", p);
                            else if (event->mask & IN_MOVE_SELF)
                            {
                                _events.emit("file-moved-self", p);
                                LK(_watchers_map_mtx);
                                watcher::remove(root);
                            }
                        }
                        i += sizeof(struct inotify_event) + event->len;
                    }
                }
            };

            _running = true;
            std::thread(f).detach();
        }

        void stop()
        {
            if (!_running)
            {
                lg("The watcher is not running, so no need to stop it.");
                return;
            }

            _running = false;
            close(_fd);
            _fd = -1;
        }

        void add(const std::string& path)
        {
            LK(_watchers_map_mtx);

            bool was_empty = _watchers.empty();
            if (!was_empty)
                watcher::stop();

            if (_fd == -1)  // Re-init if needed
                watcher::init();

            int wd = inotify_add_watch(_fd, path.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF);
            _watchers[path] = wd;

            watcher::start();
        }

        void remove(const std::string& path)
        {
            LK(_watchers_map_mtx);
            if (auto it = _watchers.find(path); it != _watchers.end())
            {
                inotify_rm_watch(_fd, _watchers[path]);
                _watchers.erase(it);
            }
            if (_watchers.empty())
                watcher::stop();
        }
    }

}
