#pragma once
#include <string>
#include "Events.h"

// this is an API that watch the filesystem for changes asynchronously and emit events to the user.
// the watching start automatically when you call at leat one time watcher::add()
// its stop automatically if you call watcher::remove() until there is no more path to watch...
// It use the Events API. you can reat to the events : 
//  - file-created
//  - file-modified
//  - file-deleted
//  - file-moved-from (file has beed moved from the watched dir, it contains its old filepath infos)
//  - file-moved-to (file has been add the this watched dir (but from a move command, not a copy or a touch one))
//  - moved-self (when the watched path has been moved and so is no longer valid) - when this is called the watched path is removed
//  Doing ml::watcher::events().add("file-created", []{...})...
//  Careful the events are emitted from a different thread !
//
namespace ml
{
    namespace watcher 
    {
        Events& events();
        void add(const std::string& path);
        void remove(const std::string& path);
    }
}
