#pragma once

#include "file.hh"
#include "http.hh"
#include "module.hh"

namespace blueshift::module {
	
	bool serve_static_file(http::request_header const & req, http::response_header & res, module::response_query & resq, std::string const & root, bool directory_listing = false);
	void setup_generic_error(http::response_header & res, module::response_query & resq, http::status_code);
	
};
