#include "http.hh"

char const * blueshift::http::text_for_status(status_code stat) {
	switch (stat) {
		case status_code::continue_s:
			return "Continue";
		case status_code::ok:
			return "OK";
		case status_code::not_found:
			return "Not Found";
		case status_code::im_a_teapot:
			return "I'm a teapot";
		case status_code::internal_server_error:
			return "Internal Server Error";
		case status_code::bad_gateway:
			return "Bad Gateway";
		default:
			return "Unknown Status Code";
	}
}
