#pragma once

#include <charconv>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <typeinfo>

#include "./str.h"

namespace tconv
{
    void setError(
            std::string* error,
            std::string_view message);

    inline bool parseBool(
            std::string_view text,
            bool& result,
            std::string* error);

    template<typename T>
        bool parseNumber(
                std::string_view text,
                T& result,
                std::string* error)
        {
            const std::string originalText(text);

            text = str::trim(text);

            if (text.empty())
            {
                setError(
                        error,
                        "Cannot convert an empty string to a number. Input: \"" +
                        originalText + "\", Target type: " +
                        typeid(T).name());

                result = T{};
                return false;
            }

            /*
             * std::from_chars does not consistently accept a leading
             * plus sign across all overloads and implementations.
             */
            if (text.front() == '+')
            {
                text.remove_prefix(1);

                if (text.empty())
                {
                    setError(
                            error,
                            "A plus sign alone is not a valid number. Input: \"" +
                            originalText + "\", Target type: " +
                            typeid(T).name());

                    result = T{};
                    return false;
                }
            }

            T convertedValue{};

            const char* begin = text.data();
            const char* end = begin + text.size();

            std::from_chars_result conversion{};

            if constexpr (std::is_floating_point_v<T>)
            {
                conversion = std::from_chars(
                        begin,
                        end,
                        convertedValue,
                        std::chars_format::general);
            }
            else
            {
                conversion = std::from_chars(
                        begin,
                        end,
                        convertedValue,
                        10);
            }

            if (conversion.ec == std::errc::invalid_argument)
            {
                setError(
                        error,
                        "The string is not a valid number. Input: \"" +
                        originalText + "\", Target type: " +
                        typeid(T).name());

                result = T{};
                return false;
            }

            if (conversion.ec == std::errc::result_out_of_range)
            {
                setError(
                        error,
                        "The numeric string is outside the target type range. Input: \"" +
                        originalText + "\", Target type: " +
                        typeid(T).name());

                result = T{};
                return false;
            }

            if (conversion.ec != std::errc{})
            {
                setError(
                        error,
                        "An unknown error occurred while parsing the number. Input: \"" +
                        originalText + "\", Target type: " +
                        typeid(T).name());

                result = T{};
                return false;
            }

            if (conversion.ptr != end)
            {
                setError(
                        error,
                        "The numeric string contains unexpected trailing characters. Input: \"" +
                        originalText + "\", Target type: " +
                        typeid(T).name());

                result = T{};
                return false;
            }

            result = convertedValue;
            return true;
        }

    template<typename T>
        bool numberToString(
                T value,
                std::string& result,
                std::string* error)
        {
            char buffer[128];

            std::to_chars_result conversion{};

            if constexpr (std::is_floating_point_v<T>)
            {
                conversion = std::to_chars(
                        buffer,
                        buffer + sizeof(buffer),
                        value,
                        std::chars_format::general,
                        std::numeric_limits<T>::max_digits10);
            }
            else
            {
                conversion = std::to_chars(
                        buffer,
                        buffer + sizeof(buffer),
                        value,
                        10);
            }

            if (conversion.ec != std::errc{})
            {
                setError(
                        error,
                        "Failed to convert the numeric value to a string. Value: " +
                        std::to_string(value) + ", Type: " +
                        typeid(T).name());

                result.clear();
                return false;
            }

            result.assign(buffer, conversion.ptr);
            return true;
        }

    template<typename Tout, typename Tin>
        bool numericConvert(
                const Tin& value,
                Tout& result,
                std::string* error)
        {
            using In = std::remove_cv_t<Tin>;
            using Out = std::remove_cv_t<Tout>;

            /*
             * Unsigned types are deliberately unsupported.
             */
            if constexpr (
                    (std::is_integral_v<In> && std::is_unsigned_v<In>) ||
                    (std::is_integral_v<Out> && std::is_unsigned_v<Out>))
            {
                setError(
                        error,
                        "Unsigned numeric conversions are not supported. Value: " +
                        std::to_string(value) + ", Source type: " +
                        typeid(In).name() + ", Target type: " +
                        typeid(Out).name());

                result = Out{};
                return false;
            }

            /*
             * Numeric value -> bool.
             *
             * 0 becomes false.
             * Any other value becomes true.
             */
            else if constexpr (std::is_same_v<Out, bool>)
            {
                result = value != static_cast<In>(0);
                return true;
            }

            /*
             * Bool -> numeric value.
             *
             * false becomes 0.
             * true becomes 1.
             */
            else if constexpr (std::is_same_v<In, bool>)
            {
                result = static_cast<Out>(value);
                return true;
            }

            /*
             * Signed integer -> signed integer.
             */
            else if constexpr (
                    std::is_integral_v<In> &&
                    std::is_integral_v<Out>)
            {
                if constexpr (
                        std::numeric_limits<Out>::digits >=
                        std::numeric_limits<In>::digits)
                {
                    result = static_cast<Out>(value);
                    return true;
                }
                else
                {
                    if (value <
                            static_cast<In>(
                                std::numeric_limits<Out>::lowest()) ||
                            value >
                            static_cast<In>(
                                std::numeric_limits<Out>::max()))
                    {
                        setError(
                                error,
                                "The integer value is outside the target type range. Value: " +
                                std::to_string(value) + ", Source type: " +
                                typeid(In).name() + ", Target type: " +
                                typeid(Out).name());

                        result = Out{};
                        return false;
                    }

                    result = static_cast<Out>(value);
                    return true;
                }
            }

            /*
             * Signed integer -> floating point.
             *
             * Precision may be lost, but the value must still fit.
             */
            else if constexpr (
                    std::is_integral_v<In> &&
                    std::is_floating_point_v<Out>)
            {
                const long double converted =
                    static_cast<long double>(value);

                const long double maximum =
                    static_cast<long double>(
                            std::numeric_limits<Out>::max());

                if (converted < -maximum || converted > maximum)
                {
                    setError(
                            error,
                            "The integer value is outside the floating-point target range. Value: " +
                            std::to_string(value) + ", Source type: " +
                            typeid(In).name() + ", Target type: " +
                            typeid(Out).name());

                    result = Out{};
                    return false;
                }

                result = static_cast<Out>(value);
                return true;
            }

            /*
             * Floating point -> signed integer.
             *
             * Values are rounded to the nearest integer.
             * Halfway values are rounded away from zero:
             *
             *  3.4 ->  3
             *  3.5 ->  4
             * -3.4 -> -3
             * -3.5 -> -4
             */
            else if constexpr (
                    std::is_floating_point_v<In> &&
                    std::is_integral_v<Out>)
            {
                if (!std::isfinite(value))
                {
                    setError(
                            error,
                            "NaN and infinity cannot be converted to an integer. Value: " +
                            std::to_string(value) + ", Source type: " +
                            typeid(In).name() + ", Target type: " +
                            typeid(Out).name());

                    result = Out{};
                    return false;
                }

                const long double rounded =
                    std::round(static_cast<long double>(value));

                const long double exclusiveUpperBound =
                    std::ldexp(
                            static_cast<long double>(1),
                            std::numeric_limits<Out>::digits);

                if (rounded < -exclusiveUpperBound ||
                        rounded >= exclusiveUpperBound)
                {
                    setError(
                            error,
                            "The rounded floating-point value is outside the integer target range. Value: " +
                            std::to_string(value) + ", Rounded: " +
                            std::to_string(rounded) + ", Source type: " +
                            typeid(In).name() + ", Target type: " +
                            typeid(Out).name());

                    result = Out{};
                    return false;
                }

                result = static_cast<Out>(rounded);
                return true;
            }

            /*
             * Floating point -> floating point.
             */
            else if constexpr (
                    std::is_floating_point_v<In> &&
                    std::is_floating_point_v<Out>)
            {
                if (std::isfinite(value))
                {
                    const long double converted =
                        static_cast<long double>(value);

                    const long double maximum =
                        static_cast<long double>(
                                std::numeric_limits<Out>::max());

                    if (converted < -maximum || converted > maximum)
                    {
                        setError(
                                error,
                                "The floating-point value is outside the target type range. Value: " +
                                std::to_string(value) + ", Source type: " +
                                typeid(In).name() + ", Target type: " +
                                typeid(Out).name());

                        result = Out{};
                        return false;
                    }
                }

                result = static_cast<Out>(value);
                return true;
            }

            else
            {
                setError(
                        error,
                        "Unsupported numeric conversion. Source type: " +
                        std::string(typeid(In).name()) + ", Target type: " +
                        typeid(Out).name());

                result = Out{};
                return false;
            }
        }
    /*
     * The only public interface.
     *
     * On success:
     *     - Returns the converted value.
     *     - Clears *error when error is not nullptr.
     *
     * On failure:
     *     - Returns the target type's default value.
     *     - Writes the failure message into *error when error is not nullptr.
     */
    template<typename Tin, typename Tout>
        Tout convert(
                const Tin& value,
                std::string* error = nullptr)
        {
            using In =
                std::remove_cv_t<std::remove_reference_t<Tin>>;

            using Out =
                std::remove_cv_t<std::remove_reference_t<Tout>>;

            /*
             * An empty error string means no error occurred.
             */
            if (error != nullptr)
                error->clear();

            /*
             * Same type -> same value.
             */
            if constexpr (std::is_same_v<In, Out>)
            {
                return value;
            }

            /*
             * Value -> std::string.
             */
            else if constexpr (std::is_same_v<Out, std::string>)
            {
                if constexpr (std::is_same_v<In, bool>)
                {
                    return value ? "true" : "false";
                }
                else if constexpr (
                        std::is_integral_v<In> &&
                        std::is_unsigned_v<In>)
                {
                    tconv::setError(
                            error,
                            "Unsigned numeric conversions are not supported. Source type: " +
                            std::string(typeid(In).name()));

                    return {};
                }
                else if constexpr (std::is_arithmetic_v<In>)
                {
                    std::string result;

                    tconv::numberToString(
                            value,
                            result,
                            error);

                    return result;
                }
                else
                {
                    tconv::setError(
                            error,
                            "The input type cannot be converted to a string. Source type: " +
                            std::string(typeid(In).name()));

                    return {};
                }
            }

            /*
             * std::string -> value.
             */
            else if constexpr (std::is_same_v<In, std::string>)
            {
                if constexpr (std::is_same_v<Out, bool>)
                {
                    bool result = false;

                    tconv::parseBool(
                            value,
                            result,
                            error);

                    return result;
                }
                else if constexpr (
                        std::is_integral_v<Out> &&
                        std::is_unsigned_v<Out>)
                {
                    tconv::setError(
                            error,
                            "Unsigned numeric conversions are not supported. Input: \"" +
                            value + "\", Target type: " +
                            typeid(Out).name());

                    return Out{};
                }
                else if constexpr (std::is_arithmetic_v<Out>)
                {
                    Out result{};

                    tconv::parseNumber(
                            value,
                            result,
                            error);

                    return result;
                }
                else
                {
                    tconv::setError(
                            error,
                            "A string cannot be converted to the requested type. Input: \"" +
                            value + "\", Target type: " +
                            typeid(Out).name());

                    return Out{};
                }
            }

            /*
             * Numeric value -> numeric value.
             */
            else if constexpr (
                    std::is_arithmetic_v<In> &&
                    std::is_arithmetic_v<Out>)
            {
                Out result{};

                tconv::numericConvert(
                        value,
                        result,
                        error);

                return result;
            }

            /*
             * Unsupported conversion.
             */
            else
            {
                tconv::setError(
                        error,
                        "The requested conversion is not supported. Source type: " +
                        std::string(typeid(In).name()) + ", Target type: " +
                        typeid(Out).name());

                return Out{};
            }
        }

    //return 0 for numbers, false for bool and "" for string, return v{} for others
    template <typename T>
        T asDefault(const T& v)
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                return "";
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                return false;
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                return T{0};
            }
            else
            {
                return T{};
            }
        }

    //set 0 for numbers, false for bool and "" for string, set v{} for others
    template <typename T>
        void setAsDefault(T& v)
        {
            v = asDefault(v);
        }
}


