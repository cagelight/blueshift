#pragma once

#include "module.hh"

namespace blueshift {
	
struct proxy {
	
	enum struct status : uint8_t {
		complete,
		incomplete,
		waiting,
		error_unknown,
		error_connection_closed,
		error_timed_out
	};
	
	proxy(http::request const &);
	status update();
	
};
	
}
