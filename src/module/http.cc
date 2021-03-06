#include "http.hh"

std::string blueshift::http::urldecode(std::string const & url_enc) {
	
	std::string url_dec;
// 	
	size_t is = url_enc.size();
	for (size_t i = 0; i < is; i++) {
		if (url_enc[i] == '%') {
			if (i + 2 >= is) break;
			char hv = url_enc[i + 1];
			char lv = url_enc[i + 2];
			if (hv > 57) hv -= 55; else hv -= 48;
			if (lv > 57) lv -= 55; else lv -= 48;
			url_dec += (hv << 4) | lv;
			i += 2;
			continue;
		} else url_dec += url_enc[i];
	}
	
	return url_dec;
}

char const * blueshift::http::text_for_status(status_code stat) {
	switch (stat) {
		case status_code::continue_s:
			return "Continue";
		// ...
		case status_code::ok:
			return "OK";
		case status_code::created:
			return "Created";
		case status_code::accepted:
			return "Accepted";
		case status_code::non_authorative_information:
			return "Non-Authorative Information";
		case status_code::no_content:
			return "No Content";
		case status_code::reset_content:
			return "Reset Content";
		case status_code::partial_content:
			return "Partial Content";
		// ...
		case status_code::see_other:
			return "See Other";
		case status_code::not_modified:
			return "Not Modified";
		// ...
		case status_code::bad_request:
			return "Bad Request";
		case status_code::unauthorized:
			return "Unauthorized";
		case status_code::payment_required:
			return "Payment Required";
		case status_code::forbidden:
			return "Forbidden";
		case status_code::not_found:
			return "Not Found";
		case status_code::method_not_allowed:
			return "Method Not Allowed";
		case status_code::not_acceptable:
			return "Not Acceptable";
		case status_code::proxy_authentication_required:
			return "Proxy Authentication Required";
		case status_code::request_timeout:
			return "Request Timeout";
		case status_code::conflict:
			return "Conflict";
		case status_code::gone:
			return "Gone";
		case status_code::length_required:
			return "Length Required";
		case status_code::precondition_failed:
			return "Precondition Failed";
		case status_code::payload_too_large:
			return "Payload Too Large";
		case status_code::uri_too_long:
			return "URI Too Long";
		case status_code::unsupported_media_type:
			return "Unsupported Media Type";
		case status_code::range_not_satisfiable:
			return "Range Not Satisfiable";
		case status_code::expectation_failed:
			return "Expectation Failed";
		case status_code::im_a_teapot:
			return "I'm a teapot";
		case status_code::misdirected_request:
			return "Misdirected Request";
		case status_code::unprocessable_entity:
			return "Unprocessable Entity";
		case status_code::locked:
			return "Locked";
		case status_code::failed_dependency:
			return "Failed Dependency";
		case status_code::upgrade_required:
			return "Upgrade Required";
		case status_code::precondition_required:
			return "Precondition Required";
		case status_code::too_many_requests:
			return "Too Many Requests";
		case status_code::request_header_fields_too_large:
			return "Request Header Fields Too Large";
		case status_code::unavailable_for_legal_reasons:
			return "Unavailable For Legal Reasons";
		case status_code::internal_server_error:
			return "Internal Server Error";
		case status_code::not_implemented:
			return "Not Implemented";
		case status_code::bad_gateway:
			return "Bad Gateway";
		// ...
		default:
			return "Unknown Status Code";
	}
}

std::string blueshift::http::cookie::serialize() const {
	std::string ser = "Set-Cookie: ";
	ser += name + '=' + value;
	if (domain != empty_str) ser += "; Domain=" + domain;
	if (path != empty_str) ser += "; Path=" + path;
	if (max_age > 0) ser += "; Max-Age=" + std::to_string(max_age);
	if (secure) ser += "; Secure";
	if (httponly) ser += "; HttpOnly";
	return ser;
}

void blueshift::http::request_header::clear() {
	method.clear();
	version.clear();
	path.clear();
	arguments.clear();
	fields.clear();
	is_multipart = false;
	multipart_type.clear();
	multipart_delimiter.clear();
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
	
	path = urldecode({ib, i});
	std::vector<std::string> path_split = strops::separate(path, std::string{"?"}, 1);
	path = path_split[0];
	if (path_split.size() > 1) {
		std::vector<std::string> args = strops::separate(path_split[1], std::string{"&"});
		for(std::vector<std::string>::const_iterator iter = args.begin(); iter != args.end(); iter++) {
			std::vector<std::string> pair = strops::separate(*iter, std::string{"="}, 1);
			if (pair.size() > 1) {
				arguments[pair[0]] = pair[1];
			}
		}
	}
	
	strops::trim(path, '/');
	strops::remove_duplicates(path, '/');
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
		std::string key {fkb, fke};
		if (!strcasecmp("Cookie", key.c_str())) {
			std::string cookiestr {ib, i};
			std::vector<std::string> cookie_v = strops::separate(cookiestr, std::string{";"});
			for (std::string & c : cookie_v) {
				std::vector<std::string> kv = strops::separate(c, std::string{"="}, 1);
				if (kv.size() < 2) continue;
				strops::trim(kv[0], ' ');
				strops::trim(kv[1], ' ');
				cookies[kv[0]] = kv[1];
			}
		} else {
			fields[{fkb, fke}] = { ib, i };
		}
		if (i++ == data.end() || *i != '\n' || i++ == data.end()) return status_code::bad_request;
	}
	
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

void blueshift::http::response_header::clear() {
	version = "HTTP/1.1";
	code = status_code::ok;
	fields.clear();
}

bool blueshift::http::response_header::parse_from(std::vector<char> const & data) {
	typedef std::vector<char>::const_iterator cci;
	cci i, ib = data.begin();
	for(i = ib; i != data.end() && *i != ' '; i++) {}
	if (i == data.end()) return false;
	version = {ib, i};
	if (i++ == data.end()) return false;
	ib = i;
	for(i = ib; i != data.end() && *i != ' '; i++) {}
	if (i == data.end()) return false;
	std::string code_str = {ib, i};
	code = static_cast<http::status_code>(strtoul(code_str.c_str(), nullptr, 10));
	if (i++ == data.end()) return false;
	ib = i;
	for(i = ib; i != data.end() && *i != '\r'; i++) {}
	if (i == data.end()) return false;
	if (i++ == data.end() || *i != '\n' || i++ == data.end()) return false;
	
	while (i != data.end() && *i != '\r' && i + 1 != data.end() && *(i+1) != '\n') {
		ib = i;
		for(i = ib; i != data.end() && *i != ':'; i++) {}
		if (i == data.end()) return false;
		cci fkb = ib, fke = i;
		if (i++ == data.end() || i++ == data.end()) return false;
		ib = i;
		for(i = ib; i != data.end() && *i != '\r'; i++) {}
		if (i == data.end()) return false;
		fields[ {fkb, fke} ] = { ib, i };
		if (i++ == data.end() || *i != '\n' || i++ == data.end()) return false;
	}
	
	return true;
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
	for (auto const & c : cookies) {
		auto v = c.serialize();
		r.insert(r.end(), v.begin(), v.end());
		r.insert(r.end(), "\r\n", "\r\n" + 2);
	}
	r.insert(r.end(), "\r\n", "\r\n" + 2);
	return r;
}

void blueshift::http::multipart_header::clear() {
	disposition.clear();
	name.clear();
	filename.clear();
	fields.clear();
}

bool blueshift::http::multipart_header::parse_from(std::vector<char> const & data) {
	{
		typedef std::vector<char>::const_iterator cci;
		cci bi, ei;
		bi = ei = data.begin();
		
		while (ei != data.end() && *ei != '\r' && ei + 1 != data.end() && *(ei+1) != '\n') {
			bi = ei;
			for(bi = ei; ei != data.end() && *ei != ':'; ei++) {}
			if (ei == data.end()) return false;
			cci fkb = bi, fke = ei;
			if (ei++ == data.end() || ei++ == data.end()) return false;
			bi = ei;
			for(bi = ei; ei != data.end() && *ei != '\r'; ei++) {}
			if (ei == data.end()) return false;
			fields[ {fkb, fke} ] = { bi, ei };
			if (ei++ == data.end() || *ei != '\n' || ei++ == data.end()) return false;
		}
	}
	
 	auto const & cd = fields.find("Content-Disposition");
	if (cd != fields.end()) {
		std::string::const_iterator bi, ei;
		bi = ei = cd->second.begin();
		for(bi = ei; ei != cd->second.end() && *ei != ';'; ei++) {}
		if (ei == cd->second.end()) return true;
		disposition = {bi, ei};
		while (true) {
			if (ei++ == cd->second.end()) break;
			if (*ei == ' ') if (ei++ == cd->second.end()) break;
			for(bi = ei; ei != cd->second.end() && *ei != '='; ei++) {}
			if (ei == cd->second.end()) break;
			std::string f {bi, ei};
			if (ei++ == cd->second.end()) break;
			for(bi = ei; ei != cd->second.end() && *ei != ';'; ei++) {}
			if (f == "name") name = {bi, ei};
			else if (f == "filename") filename = {bi, ei};
			if (ei == cd->second.end()) break;
			else continue;
		}
		strops::trim(disposition, '\"');
		strops::trim(filename, '\"');
		strops::trim(name, '\"');
	} else {
		
	}
	
	return true;
}

