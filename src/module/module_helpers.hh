#pragma once

#include "file.hh"
#include "http.hh"
#include "module.hh"

namespace blueshift::module {
	
	bool serve_static_file(http::request_header const & req, http::response_header & res, module::response_query & resq, std::string path_root, std::string const & path, std::string const & file_root, bool directory_listing = false);
	static inline bool serve_static_file(http::request_header const & req, http::response_header & res, module::response_query & resq, std::string const & root, bool directory_listing = false) {
		return serve_static_file(req, res, resq, "", req.path, root, directory_listing);
	}
	void setup_generic_error(http::response_header & res, module::response_query & resq, http::status_code);
	
};
