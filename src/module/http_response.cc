#include "http.hh"

blueshift::http::response::response() {
	set_body("TEST LOL", "text/plain");
}

void blueshift::http::response::reset() {
	body.clear();
	body.shrink_to_fit();
	body_file.close();
}

std::string blueshift::http::response::create_header() {
	
	switch (btype) {
	case body_type::buffer:
		fields["Content-Type"] = mime;
		fields["Content-Length"] = std::to_string(body.size());
		break;
	case body_type::file:
		fields["Content-Type"] = body_file.get_MIME();
		fields["Content-Length"] = std::to_string(body_file.get_size());
		break;
	case body_type::proxy:
		throw blueshift::general_exception("create_header cannot be called on an http response set for proxy mode");
	}
	
	std::string work {"HTTP/1.1 "};
	work += std::to_string(static_cast<status_code_ut>(this->status));
	work += " ";
	work += http::text_for_status(this->status);
	work += "\r\n";
	for (auto f : fields) {
		work += f.first;
		work += ": ";
		work += f.second;
		work += "\r\n";
	}
	work += "\r\n";
	return work;
}


void blueshift::http::response::set_body(char const * nb, char const * MIME) {
	size_t nbs = strlen(nb);
	body.clear();
	body.insert(body.end(), nb, nb + nbs);
	mime = MIME;
	
	btype = body_type::buffer;
}

void blueshift::http::response::set_body(std::vector<char> const & nb, char const * MIME) {
	body = nb;
	mime = MIME;
	
	btype = body_type::buffer;
}

void blueshift::http::response::set_body(blueshift::file && f) {
	if (f.get_status() != file::status::file) throw blueshift::general_exception("response body file MUST be a valid regular file!");
	body_file = std::move(f);
	btype = body_type::file;
}

void blueshift::http::response::set_body_generic() {
	std::string nb = blueshift::strf("<h1>%hu %s</h1>", status, text_for_status(status));
	set_body(nb.c_str(), "text/html");
}

void blueshift::http::response::use_proxy(char const * host, uint16_t port) {
	btype = body_type::proxy;
	proxy_host = host;
	proxy_service = std::to_string(port);
}
