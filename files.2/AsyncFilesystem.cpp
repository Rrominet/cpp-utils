#include "./AsyncFilesystem.h"
#include "./files.h"

namespace ml
{
    std::string FileObject::asString() const
    {
        return std::string(_buffer.begin(), _buffer.end());
    }

    std::vector<unsigned char> FileObject::asVector() const
    {
        return _buffer;
    }

    void FileObject::set(void* data,size_t size)
    {
        _buffer = std::vector<unsigned char>((unsigned char*)data, (unsigned char*)data + size);
    }

    void FileObject::set(const std::string& str)
    {
        _buffer = std::vector<unsigned char>(str.begin(), str.end());
    }

    void FileObject::set(const std::vector<unsigned char>& vec)
    {
        _buffer = vec;
    }

    AsyncFilesystem::AsyncFilesystem(const std::string& root) : _root(root)
    {
        if (_root.back() == files::sep())
            _root.pop_back();
    }

    void AsyncFilesystem::write(const std::string& path, const std::string& content, const std::function<void (size_t written)>& callback, const std::function<void (const std::string&)>& error)
    {
        auto result = std::make_shared<Ret<size_t>>();
        auto func = [this, path, content, result]()
        {
            *result = write_sync(path, content);
        };

        auto cb = [callback, error, result]()
        {
            if (result->success)
            {
                if (callback)
                    callback(result->value);
            }
            else if (error)
                error(result->message);
        };

        pool().run(func, cb);
    }

    void AsyncFilesystem::write(const std::string& path, const std::vector<unsigned char>& content, const std::function<void (size_t written)>& callback, const std::function<void (const std::string&)>& error)
    {
        auto result = std::make_shared<Ret<size_t>>();
        auto func = [this, path, content, result]()
        {
            *result = write_sync(path, content);
        };

        auto cb = [callback, error, result]()
        {
            if (result->success)
            {
                if (callback)
                    callback(result->value);
            }
            else if (error)
                error(result->message);
        };

        pool().run(func, cb);
    }

    void AsyncFilesystem::write(const std::string& path, void* content, size_t size, const std::function<void (size_t written)>& callback, const std::function<void (const std::string&)>& error)
    {
        auto result = std::make_shared<Ret<size_t>>();
        auto func = [this, path, content, result, size]()
        {
            *result = write_sync(path, content, size);
        };

        auto cb = [callback, error, result]()
        {
            if (result->success)
            {
                if (callback)
                    callback(result->value);
            }
            else if (error)
                error(result->message);
        };

        pool().run(func, cb);
    }

    Ret<size_t> AsyncFilesystem::write_sync(const std::string& path, const std::string& content)
    {
        _pushData(path, content);
        {
            std::lock_guard<std::mutex> lock(_beingWritten_mtx);
            try
            {
                _beingWritten.push_back(path);
                auto success = files::write(this->fullpath(path), content);
                _beingWritten.pop_back();
                return ml::ret::success<size_t>(success);
            }
            catch (const std::exception& e)
            {
                _beingWritten.pop_back();
                return ml::ret::fail<size_t>(e.what());
            }
        }
    }

    Ret<size_t> AsyncFilesystem::write_sync(const std::string& path, const std::vector<unsigned char>& content)
    {
        _pushData(path, content);
        {
            std::lock_guard<std::mutex> lock(_beingWritten_mtx);
            try
            {
                _beingWritten.push_back(path);
                auto success = files::write(this->fullpath(path), content);
                _beingWritten.pop_back();
                return ml::ret::success<size_t>(success);
            }
            catch (const std::exception& e)
            {
                _beingWritten.pop_back();
                return ml::ret::fail<size_t>(e.what());
            }
        }
    }

    Ret<size_t> AsyncFilesystem::write_sync(const std::string& path, void* content, size_t size)
    {
        _pushData(path, content, size);
        {
            std::lock_guard<std::mutex> lock(_beingWritten_mtx);
            try
            {
                _beingWritten.push_back(path);
                auto success = files::write(this->fullpath(path), content, size);
                _beingWritten.pop_back();
                return ml::ret::success<size_t>(success);
            }
            catch (const std::exception& e)
            {
                _beingWritten.pop_back();
                return ml::ret::fail<size_t>(e.what());
            }
        }
    }

    std::string AsyncFilesystem::fullpath(const std::string& path)
    {
        if (path[0] == files::sep())        
            return _root + path;
        else 
            return _root + files::sep() + path;
    }

    void AsyncFilesystem::_pushData(const std::string& path, const std::string& data)
    {
        FileObject obj;
        obj.set(data);
        {
            std::lock_guard<std::mutex> lock(_cache_mtx);
            _cache[path] = std::move(obj);
        }
    }
    void AsyncFilesystem::_pushData(const std::string& path, const std::vector<unsigned char>& data)
    {
        FileObject obj;
        obj.set(data);
        {
            std::lock_guard<std::mutex> lock(_cache_mtx);
            _cache[path] = std::move(obj);
        }
    }
    void AsyncFilesystem::_pushData(const std::string& path, void* content, size_t size)
    {
        FileObject obj;
        obj.set(content, size);
        {
            std::lock_guard<std::mutex> lock(_cache_mtx);
            _cache[path] = std::move(obj);
        }
    }

    void AsyncFilesystem::read(const std::string& path, const std::function<void (std::string)>& callback, const std::function<void (const std::string&)>& error)
    {
        auto result = std::make_shared<Ret<std::string>>();
        auto func = [this, path, result]()
        {
            *result = read_sync(path);
        };

        auto cb = [callback, error, result]()
        {
            if (result->success)
            {
                if (callback)
                    callback(result->value);
            }
            else if (error)
                error(result->message);
        };

        pool().run(func, cb);
    }

    void AsyncFilesystem::read(const std::string& path, const std::function<void (const std::vector<unsigned char>&)>& callback, const std::function<void (const std::string&)>& error)
    {
        auto result = std::make_shared<Ret<std::vector<unsigned char>>>();
        auto func = [this, path, result]()
        {
            *result = readb_sync(path);
        };

        auto cb = [callback, error, result]()
        {
            if (result->success)
            {
                if (callback)
                    callback(result->value);
            }
            else if (error)
                error(result->message);
        };

        pool().run(func, cb);
    }

    Ret<std::string> AsyncFilesystem::read_sync(const std::string& path)
    {
        {
            std::lock_guard<std::mutex> lock(_cache_mtx);
            auto it = _cache.find(path);
            if (it != _cache.end())
                return ml::ret::success<std::string>(it->second.asString());
        }

        try
        {
            std::lock_guard<std::mutex> lock(_beingWritten_mtx);
            if (std::find(_beingWritten.begin(), _beingWritten.end(), path) != _beingWritten.end())
                return ml::ret::fail<std::string>("File is being written.");
            auto content = files::read(this->fullpath(path));
            _pushData(path, content);
            return ml::ret::success<std::string>(content);
        }
        catch (const std::exception& e)
        {
            return ml::ret::fail<std::string>(e.what());
        }
    }
    Ret<std::vector<unsigned char>> AsyncFilesystem::readb_sync(const std::string& path)
    {
        {
            std::lock_guard<std::mutex> lock(_cache_mtx);
            auto it = _cache.find(path);
            if (it != _cache.end())
                return ml::ret::success<std::vector<unsigned char>>(it->second.asVector());
        }

        try
        {
            std::lock_guard<std::mutex> lock(_beingWritten_mtx);
            if (std::find(_beingWritten.begin(), _beingWritten.end(), path) != _beingWritten.end())
                return ml::ret::fail<std::vector<unsigned char>>("File is being written.");
            auto content = files::bContent(this->fullpath(path));
            _pushData(path, content);
            return ml::ret::success<std::vector<unsigned char>>(content);
        }
        catch (const std::exception& e)
        {
            return ml::ret::fail<std::vector<unsigned char>>(e.what());
        }
    }
}
