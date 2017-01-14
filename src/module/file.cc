#include "file.hh"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <unordered_map>

static std::unordered_map<std::experimental::string_view, char const *> const ext_MIME_map = {
	{"js", 			"application/javascript"},
	{"json",		"application/json"},
	{"gif", 		"image/gif"},
	{"jpg", 		"image/jpeg"},
	{"jpeg",		"image/jpeg"},
	{"png", 		"image/png"},
	{"svg", 		"image/svg+xml"},
	{"css",			"text/css"},
	{"htm", 		"text/html"},
	{"html", 		"text/html"},
	{"txt", 		"text/plain"},
};

char const * blueshift::get_extension(char const * path) {
	char const * ext = strrchr(path, '.');
	if (!ext) return path;
	ext++;
	return ext;
}

char const * blueshift::MIME_from_file_extension(char const * ext) {
	auto f = ext_MIME_map.find(ext);
	if (f != ext_MIME_map.end()) return f->second;
	return "application/octet-stream";
}

blueshift::file::file(char const * path) {
	fd = open(path, O_RDONLY);
	
	if (fd < 0) return;
	
	struct stat finfo;
	fstat(fd, &finfo);
	
	if (S_ISREG(finfo.st_mode)) {
		status_ = status::file;
		last_modified = finfo.st_mtim;
		length = finfo.st_size;
		mime_type = MIME_from_file_extension(get_extension(path));
	} else if (S_ISDIR(finfo.st_mode)) {
		status_ = status::directory;
	} else return;
	
} 

blueshift::file::~file() {
	close();
} 

blueshift::file & blueshift::file::operator = (file && last) {
	fd = last.fd;
	status_ = last.status_;
	last_modified = last.last_modified;
	mime_type = std::move(last.mime_type);
	length = last.length;
	last.fd = -1;
	return *this;
} 

void blueshift::file::close() {
	if (fd >= 0) ::close(fd);
	fd = -1;
}
