#include "http.hh"

void blueshift::http::response_header::clear() {
	version.clear();
	code = status_code::internal_server_error;
	fields.clear();
}

bool blueshift::http::response_header::parse_from(std::vector<char> const & data) {
	return false;
}

std::vector<char> blueshift::http::response_header::serialize() {
	std::vector<char> r;
	r.insert(r.end(), version.begin(), version.end());
	r.push_back(' ');
	std::string code_str = std::to_string(static_cast<uint_fast16_t>(code));
	r.insert(r.end(), code_str.begin(), code_str.end());
	r.push_back(' ');
	code_str = text_for_status(code);
	r.insert(r.end(), code_str.begin(), code_str.end());
	r.insert(r.end(), "\r\n", "\r\n" + 2);
	for (auto const & i : fields) {
		r.insert(r.end(), i.first.begin(), i.first.end());
		r.insert(r.end(), ": ", ": " + 2);
		r.insert(r.end(), i.second.begin(), i.second.end());
		r.insert(r.end(), "\r\n", "\r\n" + 2);
	}
	r.insert(r.end(), "\r\n", "\r\n" + 2);
	return r;
}
