#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <vector>

namespace blueshift {
	
	extern std::atomic_bool run_sem;
	
	typedef std::runtime_error general_exception;
	typedef unsigned char byte;

	struct byte_buffer {
		byte_buffer() = default;
		byte_buffer(byte_buffer const & other) = default;
		byte_buffer(byte_buffer && other) = default;
		byte_buffer(std::string const & str) { buf_.resize(str.size()); memcpy(buf_.data(), str.data(), str.size()); }
		~byte_buffer() = default;
		typedef std::vector<byte>::iterator iterator;
		typedef std::vector<byte>::const_iterator const_iterator;
		std::string hex () const;
		std::string hexlow () const;
		inline size_t size () const { return buf_.size(); }
		inline byte * data () { return buf_.data(); }
		inline byte const * data () const { return buf_.data(); }
		inline void reserve (size_t s) {buf_.reserve(s); }
		inline void resize (size_t s) {buf_.resize(s); }
		inline iterator begin () { return buf_.begin(); }
		inline iterator end () { return buf_.end(); }
		inline const_iterator begin () const { return buf_.begin(); }
		inline const_iterator end () const { return buf_.end(); }
		inline iterator insert(const_iterator P, const_iterator A, const_iterator B) { return buf_.insert(P, A, B); }
		inline byte_buffer & operator = (byte_buffer const & other) = default;
		inline byte_buffer & operator = (byte_buffer && other) = default;
		inline byte_buffer operator + (byte_buffer const & other) const { byte_buffer bb {*this}; bb.insert(bb.end(), other.begin(), other.end()); return bb; }
		inline std::vector<byte> & get_buffer() {return buf_;}
		inline std::vector<byte> const & get_buffer() const {return buf_;}
	private:
		std::vector<byte> buf_;
	};
	
	std::string strf(char const * fmt, ...) noexcept;
	
	void print(char const *) noexcept;
	void print(char *) noexcept;
	inline void print(std::string const & str) noexcept { print(str.c_str()); }
	inline void print(byte_buffer const & bb) noexcept { print(bb.hex()); }
	template <typename T> void print(T t) noexcept { print(std::to_string(t)); }
	template <typename ... T> void printf(char const * fmt, T ... t) noexcept { print(strf(fmt, t ...)); }
	
	static std::string const empty_str {""};
}


#define srcprintf(lev, fmt, ...) blueshift::print(blueshift::strf("%s (%s, line %u): %s", #lev, __PRETTY_FUNCTION__, __LINE__, blueshift::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#define srcprintf_warning(fmt, ...) blueshift::print(blueshift::strf("WARNING (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, blueshift::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#define srcprintf_error(fmt, ...) blueshift::print(blueshift::strf("ERROR (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, blueshift::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#define srcthrow(fmt, ...) throw blueshift::general_exception(blueshift::strf("ERROR (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, blueshift::strf(fmt, ##__VA_ARGS__).c_str()))
#ifdef BLUESHIFT_DEBUG
#define srcprintf_debug(fmt, ...) blueshift::print(blueshift::strf("DEBUG (%s, line %u): %s", __PRETTY_FUNCTION__, __LINE__, blueshift::strf(fmt, ##__VA_ARGS__).c_str()).c_str())
#else
#define srcprintf_debug(fmt, ...)
#endif
