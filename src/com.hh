#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <memory>

namespace com {
	
	extern std::atomic_bool run_sem;
	
	typedef std::runtime_error general_exception;
	
	std::string strf(char const * fmt, ...) noexcept;
	
	void print(char const *) noexcept;
	inline void print(std::string const & str) noexcept { print(str.c_str()); }
	template <typename T> void print(T t) noexcept { print(std::to_string(t)); }
	template <typename ... T> void printf(char const * fmt, T ... t) noexcept { print(strf(fmt, t ...)); }
	
}

#define srcprintf(lev, fmt, ...) com::print(com::strf("%s (%s, line %u): %s", #lev, __PRETTY_FUNCTION__, __LINE__, com::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#define srcprintf_error(fmt, ...) com::print(com::strf("ERROR (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, com::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#define srcthrow(fmt, ...) throw com::general_exception(com::strf("ERROR (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, com::strf(fmt, ##__VA_ARGS__).c_str()))
#ifdef VALORIC_DEBUG
#define srcprintf_debug(fmt, ...) com::print(com::strf("DEBUG (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, com::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#else
#define srcprintf_debug(fmt, ...)
#endif
