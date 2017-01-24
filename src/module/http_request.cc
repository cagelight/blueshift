#include "http.hh"

void blueshift::http::request_header::clear() {
	method.clear();
	version.clear();
	path.clear();
	fields.clear();
}

blueshift::http::status_code blueshift::http::request_header::parse_from(std::vector<char> const & data) {
	typedef std::vector<char>::const_iterator cci;
	cci i, ib = data.begin();
	for(i = ib; i != data.end() && *i != ' '; i++) {}
	if (i == data.end()) return status_code::bad_request;
	method = {ib, i};
	if (i++ == data.end()) return status_code::bad_request;
	ib = i;
	for(i = ib; i != data.end() && *i != ' '; i++) {}
	if (i == data.end()) return status_code::bad_request;
	path = {ib, i};
	if (i++ == data.end()) return status_code::bad_request;
	ib = i;
	for(i = ib; i != data.end() && *i != '\r'; i++) {}
	if (i == data.end()) return status_code::bad_request;
	version = {ib, i};
	if (i++ == data.end() || *i != '\n' || i++ == data.end()) return status_code::bad_request;
	
	while (i != data.end() && *i != '\r' && i + 1 != data.end() && *(i+1) != '\n') {
		ib = i;
		for(i = ib; i != data.end() && *i != ':'; i++) {}
		if (i == data.end()) return status_code::bad_request;
		cci fkb = ib, fke = i;
		if (i++ == data.end() || i++ == data.end()) return status_code::bad_request;
		ib = i;
		for(i = ib; i != data.end() && *i != '\r'; i++) {}
		if (i == data.end()) return status_code::bad_request;
		fields[ {fkb, fke} ] = { ib, i };
		if (i++ == data.end() || *i != '\n' || i++ == data.end()) return status_code::bad_request;
	}
	
	if (method != "GET") return status_code::method_not_allowed;
	if (version != "HTTP/1.1") return status_code::http_version_not_supported;
	
	return status_code::ok;
}

std::vector<char> blueshift::http::request_header::serialize() {
	std::vector<char> r;
	r.insert(r.end(), method.begin(), method.end());
	r.push_back(' ');
	r.insert(r.end(), path.begin(), path.end());
	r.push_back(' ');
	r.insert(r.end(), version.begin(), version.end());
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
