#pragma once
#include <memory>

template<typename T>
using _ptr = std::shared_ptr<T>;
template<typename T>
using w_ptr = std::weak_ptr<T>;
template<typename T>
using u_ptr = std::unique_ptr<T>;

template<typename T, typename... Args>
    auto ptr(Args&&... args)
    {
        return std::make_shared<T>(args...);
    }

template<typename T, typename... Args>
    auto uptr(Args&&... args)
    {
        return std::make_unique<T>(args...);
    }

template<typename T, typename... Args>
    auto spc(Args&&... args)
    {
        return std::static_pointer_cast<T>(args...);
    }

template<typename T, typename... Args>
    auto dpc(Args&&... args)
    {
        return std::dynamic_pointer_cast<T>(args...);
    }
