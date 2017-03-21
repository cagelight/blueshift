#include "com.hh"

#include <cstdarg>
#include <csignal>

static constexpr uint8_t hmask = 0b11110000;
static constexpr uint8_t lmask = 0b00001111;

std::string blueshift::byte_buffer::hex() const {
	std::string ret {};
	ret.reserve(buf_.size() * 2);
	for (uint8_t ch : buf_) {
		char vh = (ch & hmask) >> 4;
		char vl = ch & lmask;
		ret += vh > 9 ? 55 + vh : 48 + vh;
		ret += vl > 9 ? 55 + vl : 48 + vl;
	}
	return ret;
}

std::string blueshift::byte_buffer::hexlow() const {
	std::string ret {};
	ret.reserve(buf_.size() * 2);
	for (uint8_t ch : buf_) {
		char vh = (ch & hmask) >> 4;
		char vl = ch & lmask;
		ret += vh > 9 ? 87 + vh : 48 + vh;
		ret += vl > 9 ? 87 + vl : 48 + vl;
	}
	return ret;
}

static constexpr size_t strf_startlen = 256;
std::string blueshift::strf(char const * fmt, ...) noexcept {
	va_list va;
	va_start(va, fmt);
	char * tmp_buf = reinterpret_cast<char *>(malloc(strf_startlen));
	tmp_buf[strf_startlen - 1] = 0;
	size_t req_size = vsnprintf(tmp_buf, strf_startlen, fmt, va);
	va_end(va);
	if (req_size >= strf_startlen) {
		tmp_buf = reinterpret_cast<char *>(realloc(tmp_buf, req_size+1));
		va_start(va, fmt);
		vsprintf(tmp_buf, fmt, va);
		va_end(va);
		tmp_buf[req_size] = 0;
	}
	std::string r = {tmp_buf};
	free(tmp_buf);
	return {r};
}

void blueshift::print(char const * str) noexcept {
	std::printf("%s\n", str);
}

void blueshift::print(char * str) noexcept {
	std::printf("%s\n", str);
}
