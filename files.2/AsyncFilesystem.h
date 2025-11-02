#pragma once
#include "../str.h"
#include "../Ret.h"
#include <unordered_map>
#include "../thread.h"


#ifdef mlipc
#include "../stds.h"
#include "../ipc.h"
#endif

// all write and read are async by default.
// The callback are called from the main thread when you call _pool.processCallbacks()
// Tipicly, put it in your event loop ! and Use the signal equivalent of your event loop to wake up it with _pool.setWakeUpFunc()
// you have _sync version if you need to wait until the operation is finished.
// You have helper function that do this for you if you use eventloop in mlapi ou mlgui.2
//
// the read read from RAM cache if it already exists.
namespace ml
{
    class FileObject
    {
        public :
            FileObject() = default;
            virtual ~FileObject() = default;

            std::string asString() const;
            std::vector<unsigned char> asVector() const;

            // the set function COPY the data, it's not a pointer and it's mendatory to work.
            void set(void* data, size_t size);
            void set(const std::string& str);
            void set(const std::vector<unsigned char>& vec);

        protected : 
            std::vector<unsigned char> _buffer;
    };

    class AsyncFilesystem
    {
        public : 
            AsyncFilesystem(const std::string& root);
            virtual ~AsyncFilesystem() = default;

            void write(const std::string& path, const std::string& content, const std::function<void (size_t written)>& callback=0, const std::function<void (const std::string&)>& error=0);
            void write(const std::string& path, const std::vector<unsigned char>& content, const std::function<void (size_t written)>& callback=0, const std::function<void (const std::string&)>& error=0);
            void write(const std::string& path, void* content, size_t size, const std::function<void (size_t written)>& callback=0, const std::function<void (const std::string&)>& error=0);

            Ret<size_t> write_sync(const std::string& path, const std::string& content);
            Ret<size_t> write_sync(const std::string& path, const std::vector<unsigned char>& content);
            Ret<size_t> write_sync(const std::string& path, void* content, size_t size);

            //the callback will be called on the same thread as the caller.
            //So it's usable in a GUI application without needing to doing the queuing yourself.
            void read(const std::string& path, const std::function<void (std::string)>& callback=0, const std::function<void (const std::string&)>& error=0);
            void read(const std::string& path, const std::function<void (const std::vector<unsigned char>&)>& callback=0, const std::function<void (const std::string&)>& error=0);
            Ret<std::string> read_sync(const std::string& path);
            Ret<std::vector<unsigned char>> readb_sync(const std::string& path);


#ifdef mlipc
            //this helper function set all the event loop and callback system for the ipc stdin read loop in ipc.h
            //you need to define mlipc before including this file.
        void setForIPC()
        {
            stds::init();
            _pool.setWakeupFunc(ipc::signal);
            ipc::addOnReadLoop([this](){_pool.processCallbacks();});
        }
#endif

            template<typename App>
                //require the lib mlgui.2 running a GUI event loop (typically, creating an App instance)
                //the App is the ml::app() instance return globally by mlgui.2
                void setForMlGui(App* app)
            {
                _pool.setWakeupFunc([app, this]{app->queue([this]{_pool.processCallbacks();});});
            }

            th::ThreadPool& pool() {return _pool;}
            std::string root() {return _root;}
            std::string fullpath(const std::string& path);

        protected:
            std::string _root;
            th::ThreadPool _pool;

            // the filepath key is from the root
            std::unordered_map<std::string, FileObject> _cache;
            std::mutex _cache_mtx;

            std::vector<std::string> _beingWritten;
            std::mutex _beingWritten_mtx;

            void _pushData(const std::string& path, const std::string& data);
            void _pushData(const std::string& path, const std::vector<unsigned char>& data);
            void _pushData(const std::string& path, void* content, size_t size);
    };
}
