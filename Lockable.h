#pragma once
#include <mutex>

namespace ml
{
    // you can simply getthe value in it calling inst.get() or inst.v()
    // the lock/unlock is meant to be used by the end user (with a lock_guard for example)
    template <typename T, typename Mtx=std::mutex>
    class Lockable
    {
        public : 
            template <typename ...Args>
            Lockable(Args&& ...args) : _value(std::forward<Args>(args)...) {}

            void lock() const { _mutex.lock(); }
            void unlock() const { _mutex.unlock(); }

            T& value() { return _value; }
            const T& value() const { return _value; }

            T& v() { return _value; }
            const T& v() const { return _value; }

            T& get() { return _value; }
            const T& get() const { return _value; }

            operator T& () { return _value; }
            operator const T& () { return _value; }

            Lockable& operator = (const T& value) { _value = value; return *this; }
            Lockable& operator = (T&& value) noexcept { _value = std::move(value); return *this; }

            // this copy the value without the attached mutex.
            T copy() const { return _value; }

        private : 
            T _value;
            mutable Mtx _mutex;
    };
}
