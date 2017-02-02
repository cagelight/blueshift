#include "file.hh"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

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
	{"mp4",			"video/mp4"},
	{"webm",		"video/webm"},
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

struct file_data {
	
	file_data() = default;
	~file_data() {
		if (fd >= 0) {
			close(fd);
		}
	}
	
	int fd = -1;
	struct stat finfo {};
};

#define fdata reinterpret_cast<file_data *>(internal_data)

blueshift::file::file(std::string const & path) {
	
	internal_data = new file_data {};
	
	fdata->fd = ::open(path.c_str(), O_RDONLY); 
	if (fdata->fd < 0) return;
	
	fstat(fdata->fd, &fdata->finfo);
	
	if (S_ISREG(fdata->finfo.st_mode)) {
		type_ = type::file;
		mime_type = MIME_from_file_extension(get_extension(path.c_str()));
	} else if (S_ISDIR(fdata->finfo.st_mode)) {
		type_ = type::directory;
		//fdata->dir = fdopendir(fdata->fd);
	} else return;
	
} 

blueshift::file::~file() {
	delete fdata;
} 

int blueshift::file::get_FD() const {
	return fdata->fd;
}

size_t blueshift::file::get_size() const {
	return fdata->finfo.st_size;
}

blueshift::realtime_clock::time_point blueshift::file::get_last_modified() const {
	return {fdata->finfo.st_mtim};
}

std::vector<blueshift::directory_listing> blueshift::file::get_files() const {
	
	if (type_ != type::directory) srcthrow("is not directory, what are you even doing");
	std::vector<directory_listing> r {};
	
	DIR * d = fdopendir(fdata->fd);
	if (!d) return r;
	
	for (dirent * entry = readdir(d); entry != nullptr; entry = readdir(d)) {
		directory_listing dl {entry->d_name, type::invalid};
		if (entry->d_type == DT_REG) dl.type_ = type::file;
		else if (entry->d_type == DT_DIR) dl.type_ = type::directory;
		r.push_back(dl);
	}
	
	return r;
}

