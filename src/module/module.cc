#include "module.hh"

void blueshift::module::request_query::set_token(void * token) {
	processing_token = token;
}

void blueshift::module::request_query::refuse_payload() {
	q = qe::refuse_payload;
}

void blueshift::module::request_query::use_proxy_direct(const char* ip_addr, uint16_t port) {
	q = qe::proxy_direct;
	proxy_host = ip_addr;
	proxy_port = port;
}


void blueshift::module::request_query::use_proxy_resolve(const char* host, const char* service) {
	q = qe::proxy_resolve;
	proxy_host = host;
	proxy_service = service;
}

void blueshift::module::request_query::reset() {
	proxy_host.clear();
	proxy_service.clear();
	proxy_port = 0;
	q = qe::ok;
	processing_token = nullptr;
}

void blueshift::module::response_query::set_body(std::vector<char> const & buf, char const * MIMEi) {
	body_buffer = buf;
	MIME = MIMEi;
	q = qe::body_buffer;
}

void blueshift::module::response_query::set_body(std::vector<char> && buf, char const * MIMEi) {
	body_buffer = std::move(buf);
	MIME = MIMEi;
	q = qe::body_buffer;
}

void blueshift::module::response_query::set_body(shared_file const & fil, char const * MIMEi) {
	body_file = fil;
	MIME = MIMEi ? MIMEi : body_file->get_MIME();
	q = qe::body_file;
}

void blueshift::module::response_query::reset() {
	body_buffer.clear();
	body_buffer.shrink_to_fit();
	body_file.reset();
	MIME.clear();
	q = qe::no_body;
}
