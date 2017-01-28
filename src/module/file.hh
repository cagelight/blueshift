#pragma once
#include "com.hh"
#include "time.hh"

#include <vector>
#include <experimental/string_view> // TODO -- string_view was accepted into C++17, but was not available at the time of writing, switch when available

namespace blueshift {
	
char const * get_extension(char const * path);
char const * MIME_from_file_extension(char const * ext);

typedef std::shared_ptr<struct file> shared_file;

struct directory_listing;

struct file {
	
	enum struct type : uint_fast8_t {
		invalid, // unknown or not found
		file,
		directory
	};
	
	static shared_file open(std::string const & path) { return shared_file {new file {path}}; }
	
	file() = delete;
	file(std::string const & path);
	~file();
	
	file (file const &) = delete;
	file (file &&) = delete;
	
	file & operator = (file const &) = delete;
	file & operator = (file &&);

	int get_FD() const;
	type get_type() const { return type_; }
	std::string const & get_MIME() const { return mime_type; }
	size_t get_size() const;
	time::point get_last_modified() const;
	std::vector<directory_listing> get_files() const;
	
private:
	
	void * internal_data = nullptr;
	type type_ = type::invalid;
	std::string mime_type = "application/octet-stream";
};

struct directory_listing {
	std::string name;
	file::type type_;
	
	shared_file get_file() { return file::open(name); }
	
	struct natsortcmp_s {
		bool operator() (directory_listing const & a, directory_listing const & b) {
			if (a.type_ < b.type_) return false;
			if (a.type_ > b.type_) return true;
			return strops::natcmp<std::string::const_iterator>(a.name.begin(), a.name.end(), b.name.begin(), b.name.end());
		}
	};
};


}
