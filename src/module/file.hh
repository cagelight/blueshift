#pragma once
#include "com.hh"

#include <ctime>

#include <experimental/string_view> // TODO -- string_view was accepted into C++17, but was not available at the time of writing, switch when available

namespace blueshift {
	
char const * get_extension(char const * path);
char const * MIME_from_file_extension(char const * ext);

typedef std::shared_ptr<struct file> shared_file;

struct file {
	
	enum struct status : uint_fast8_t {
		invalid,
		file,
		directory
	};
	
	static shared_file open(char const * path) { return shared_file {new file {path}}; }
	
	file() = delete;
	file(char const * path);
	~file();
	
	file (file const &) = delete;
	file (file &&) = delete;
	
	file & operator = (file const &) = delete;
	file & operator = (file &&);
	
	void close();
	
	int get_FD() const { return fd; }
	status get_status() const { return status_; }
	std::string const & get_MIME() const { return mime_type; }
	size_t get_size() const { return length; }
	
private:
	
	int fd = -1;
	status status_ = status::invalid;
	struct timespec last_modified;
	std::string mime_type = "application/octet-stream";
	size_t length = 0;
};

}
