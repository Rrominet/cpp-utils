#include "watcher.h"
#include <unordered_map>
#include <sys/inotify.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <queue>
#include "./files.h"
#include "../debug.h"
#include "../thread.h"

namespace ml
{
    namespace watcher 
    {
        struct WatcherSync
        {
            bool should_exit = false;
            int fd = -1;
            std::queue<std::string> pending_removals;
            std::unordered_map<std::string, int> watchers;
            bool thread_running = false;
        };

        th::Safe<WatcherSync> _watcher_sync ("Dir Watcher");

        Events _events;
        Events& events(){return _events;}

        void init()
        {
            if (_watcher_sync.data().fd != -1) return;  // Already initialized

            _watcher_sync.data().fd = inotify_init();
            if (_watcher_sync.data().fd < 0)
            {
                perror("inotify_init");
                throw std::runtime_error("Can't init inotify...");
            }
        }

        void watcher_thread_func()
        {
            char buffer[8192];

            while(true)
            {
                int fd_copy;
                {
                    std::lock_guard l(_watcher_sync);
                    if (_watcher_sync.data().should_exit)
                        break;
                    fd_copy = _watcher_sync.data().fd;

                    // Process pending removals
                    while (!_watcher_sync.data().pending_removals.empty())
                    {
                        std::string path = _watcher_sync.data().pending_removals.front();
                        _watcher_sync.data().pending_removals.pop();

                        if (auto it = _watcher_sync.data().watchers.find(path); it != _watcher_sync.data().watchers.end())
                        {
                            inotify_rm_watch(_watcher_sync.data().fd, it->second);
                            _watcher_sync.data().watchers.erase(it);
                        }
                    }
                }

                if (fd_copy == -1) break;

                int length = read(fd_copy, buffer, sizeof(buffer));

                if (length == -1) 
                {
                    if (errno == EBADF || errno == EINTR) 
                    {
                        lg("inotify fd closed or interrupted, stopping watcher thread...");
                        break;
                    }
                    continue;
                }

                int i = 0;
                while(i < length)
                {
                    struct inotify_event *event = (struct inotify_event *)&buffer[i];

                    if (event->len) 
                    {
                        std::string root;
                        {
                            std::lock_guard l(_watcher_sync);
                            for (auto& w : _watcher_sync.data().watchers)
                            {
                                if (w.second == event->wd)
                                {
                                    root = w.first;
                                    break;
                                }
                            }
                        }

                        if (!root.empty())
                        {
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
                                _events.emit("file-moved-self", root);

                                // Queue removal instead of direct call to avoid deadlock
                                std::lock_guard l(_watcher_sync);
                                _watcher_sync.data().pending_removals.push(root);
                            }
                        }
                    }
                    i += sizeof(struct inotify_event) + event->len;
                }
            }

            std::lock_guard l(_watcher_sync);
            _watcher_sync.data().thread_running = false;
        }

        void start()
        {
            std::lock_guard l(_watcher_sync);

            if (_watcher_sync.data().thread_running) return;  // Thread already running

            _watcher_sync.data().should_exit = false;
            _watcher_sync.data().thread_running = true;
            std::thread(watcher_thread_func).detach();
        }

        void stop()
        {
            std::lock_guard l(_watcher_sync);

            if (!_watcher_sync.data().thread_running)
                return;

            _watcher_sync.data().should_exit = true;

            if (_watcher_sync.data().fd != -1)
            {
                close(_watcher_sync.data().fd);
                _watcher_sync.data().fd = -1;
            }

            // Thread will clean up _thread_running flag itself
        }

        void add(const std::string& path, bool onlyOne)
        {
            {
                std::lock_guard l(_watcher_sync);

                // Your stupid onlyOne logic
                if (onlyOne && !_watcher_sync.data().watchers.empty())
                {
                    // Clear all existing watcher_sync.data().watchers
                    for (auto& w : _watcher_sync.data().watchers)
                    {
                        inotify_rm_watch(_watcher_sync.data().fd, w.second);
                    }
                    _watcher_sync.data().watchers.clear();

                    // Stop and reinit
                    if (_watcher_sync.data().fd != -1)
                    {
                        _watcher_sync.data().should_exit = true;
                        close(_watcher_sync.data().fd);
                        _watcher_sync.data().fd = -1;
                    }
                }

                if (_watcher_sync.data().fd == -1)
                {
                    init();
                }

                int wd = inotify_add_watch(_watcher_sync.data().fd, path.c_str(), 
                        IN_CREATE | IN_DELETE | IN_MODIFY | IN_ATTRIB | 
                        IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF);

                if (wd == -1)
                {
                    perror("inotify_add_watch");
                    return;
                }
                _watcher_sync.data().watchers[path] = wd;
            }
            start();
        }

        void remove(const std::string& path)
        {
            std::lock_guard l(_watcher_sync);

            if (auto it = _watcher_sync.data().watchers.find(path); it != _watcher_sync.data().watchers.end())
            {
                if (_watcher_sync.data().fd != -1)
                {
                    inotify_rm_watch(_watcher_sync.data().fd, it->second);
                }
                _watcher_sync.data().watchers.erase(it);
            }

            if (_watcher_sync.data().watchers.empty())
            {
                l.~lock_guard();
                stop();
            }
        }

        void stop_all()
        {
            {
                std::lock_guard l(_watcher_sync);

                for (auto& w : _watcher_sync.data().watchers)
                {
                    if (_watcher_sync.data().fd != -1)
                    {
                        inotify_rm_watch(_watcher_sync.data().fd, w.second);
                    }
                }
                _watcher_sync.data().watchers.clear();
            }
            stop();
        }
    }
}
