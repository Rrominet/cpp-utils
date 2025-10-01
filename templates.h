#pragma once
#include <type_traits>
#include <iterator>

namespace tpl
{
    template<typename T>
        auto& instance(const T& _instance)
        {
            if constexpr(std::is_pointer<T>::value)
                return *_instance;
            else if constexpr(!std::is_pointer<T>::value)
                return _instance;
        }

    // usage : 
    // if constexpr(isBase<BaseClass, ChildClass>())
    template<typename Base, typename T>
        constexpr bool isBase() 
        {
            return std::is_base_of<Base, T>::value;
        }

    //usage :
    // if constexpr(checkType<A, B>())
    template<typename A, typename B>
        constexpr bool checkType()
        {
            return std::is_same<A, B>::value;
        }

    template<typename A>
        constexpr bool isNumber()
        {
            if (checkType<A, int>())
                return true;
            else if (checkType<A, float>())
                return true;
            else if (checkType<A, long>())
                return true;
            else if (checkType<A, double>())
                return true;
            else if (checkType<A, unsigned int>())
                return true;
            else if (checkType<A, unsigned long>())
                return true;
            return false;
        }

    // how to check if a method exists in a class instance ?
    // considering this method : 
    // a.yourMethod(int a, double b, ml::vec2 c)
    // you need to create 2 template structs : 
    //
    // template<typename T, typename = void> // the void need to be there or else the SFINAE will not work | becquse we "compare 2 types"
    // struct has_yourMethod : std::false_type {};
    //
    // the 2nd template is simply a template thate return a true template is it form does noe produce a compile error (if it does the compiler will use the first one that return a false template)
    // template<typename T>
    // struct has_yourMethod <T, std::void_t<decltype(std::declval<T>().yourMethod(
    //   std::declval<int>(),
    //   std::declval<double>(),
    //   std::declval<ml::vec2>(),
    // ))>> : std::true_type {};
    // 
    // usage : 
    // if constexpr(has_yourMethod<YourType>::value)

    template <typename T>
    struct type_identity {
        using type = T;
    };

    template <typename T>
    using type_identity_t = typename type_identity<T>::type;

    template <typename T, typename = void>
        struct is_iterable : std::false_type {};

    template <typename T>
        struct is_iterable<
        T,
        std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>
            > : std::true_type {};

    // Utility to simplify checking
    template <typename T>
        inline constexpr bool isIterable = is_iterable<T>::value;
}
