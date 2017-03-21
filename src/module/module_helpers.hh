#pragma once

#include "file.hh"
#include "http.hh"
#include "module.hh"

namespace blueshift::module {
	
	bool serve_static_file(http::request_header const & req, http::response_header & res, module::response_query & resq, std::string path_root, std::string const & path, std::string const & file_root, bool directory_listing = false, bool htmlcheck = true);
	static inline bool serve_static_file(http::request_header const & req, http::response_header & res, module::response_query & resq, std::string const & root, bool directory_listing = false, bool htmlcheck = true) {
		return serve_static_file(req, res, resq, "", req.path, root, directory_listing, htmlcheck);
	}
	void setup_generic_error(http::response_header & res, module::response_query & resq, http::status_code, std::string const & additional_info = empty_str);
	std::unordered_map<std::string, std::string> form_urlencoded_decode(std::string enc); // x-www-form-urlencoded
};
