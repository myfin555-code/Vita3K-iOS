#pragma once
#include <fmt/format.h>
// Host test diagnostics only; production logging is not linked.
template <typename T>
std::string log_hex(T value) { return fmt::format("0x{:x}", value); }
#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARN(...) ((void)0)
#define LOG_ERROR(...) fmt::print(stderr, __VA_ARGS__)
#define LOG_CRITICAL(...) fmt::print(stderr, __VA_ARGS__)
#define LOG_CRITICAL_IF(...) ((void)0)
#define LOG_WARN_ONCE(...) ((void)0)
