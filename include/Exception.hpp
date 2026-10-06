#pragma once

#include <format>
#include <stdexcept>

namespace mg {

template<typename... Args>
void ASSERT(bool condition, std::format_string<Args...> fmt, Args&&... args)
{
    if (!condition) {
        std::string msg = std::format(fmt, std::forward<Args>(args)...);
        throw std::runtime_error(msg);
    }
}

template <typename... Args>
void INTERNAL_ASSERT_DEBUG_ONLY(
    bool condition, std::format_string<Args...> fmt, 
    Args&&... args) 
{
#ifdef MINIGRAD_DEBUG
    ASSERT(condition, fmt, loc, args);
#endif
}

}