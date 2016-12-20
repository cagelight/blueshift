#include "protocol.hh"

char const * blueshift::http::text_for_status(status stat) {
	switch (stat) {
		case status::continue_s:
			return "Continue";
		case status::ok:
			return "OK";
		case status::not_found:
			return "Not Found";
		default:
			return "Unknown Status Code";
	}
}
