#include "router.hh"
#include <module_helpers.hh>

using namespace blueshift;

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

bool router::uri_resolver::regex_route::search(std::string const & path) const {
	return std::regex_search(path, reg);
}

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

blueshift::module::mss basic_route::query(blueshift::module::request_query & reqq) {
	if (request->is_multipart) {
		reqq.refuse_payload();
		condition = blueshift::http::status_code::unsupported_media_type;
	}
	if (request->content_length() > max_content_size) {
		reqq.refuse_payload();
		condition = blueshift::http::status_code::payload_too_large;
	}
	return blueshift::module::mss::proceed;
}
void basic_route::process_begin () { }
blueshift::module::mss basic_route::process (std::vector<char> const & buffer_data) {
	body.insert(body.end(), buffer_data.begin(), buffer_data.end());
	return blueshift::module::mss::proceed;
}
void basic_route::process_end () { }
void basic_route::process_multipart_begin (blueshift::http::multipart_header const &) { }
blueshift::module::mss basic_route::process_multipart (blueshift::http::multipart_header const &, std::vector<char> const &) { return blueshift::module::mss::proceed; }
void basic_route::process_multipart_end (blueshift::http::multipart_header const &) { }
blueshift::module::mss basic_route::finalize_response (blueshift::http::response_header & res, blueshift::module::response_query & resq) {
	if (condition != blueshift::http::status_code::ok) {
		blueshift::module::setup_generic_error(res, resq, condition);
		return blueshift::module::mss::proceed;
	}
	response = &res;
	response_query = &resq;
	respond();
	return blueshift::module::mss::proceed;
}

void blueshift::basic_route::generic_error(http::status_code code) {
	module::setup_generic_error(*response, *response_query, code);
}

bool blueshift::basic_route::serve_file(std::string const & file_root, bool directory_listing, bool htmlcheck) {
	return module::serve_static_file(*request, *response, *response_query, file_root, directory_listing, htmlcheck);
}

bool blueshift::basic_route::serve_file(std::string const & path_head, std::string const & path_tail, std::string const & file_root, bool directory_listing, bool htmlcheck) {
	return module::serve_static_file(*request, *response, *response_query, path_head, path_tail, file_root, directory_listing, htmlcheck);
}

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================


struct router_session {
	::route * ptr = nullptr;
	blueshift::http::status_code condition = blueshift::http::status_code::ok;
	std::shared_ptr<router> subrouter = nullptr;
	void * subroute_session = nullptr;
	std::string adj_path = "";
	~router_session() { 
		if (subrouter && subroute_session) subrouter->cleanup(subroute_session);
		if (ptr) delete ptr;
	}
};

#define session reinterpret_cast<router_session *>(*processing_token)

blueshift::module::mss router::query (void * * processing_token, blueshift::http::request_header const & req, blueshift::module::request_query & reqq, std::string override_path) {
	
	if (session) {
		if (session->subrouter) return session->subrouter->query(&session->subroute_session, req, reqq, session->adj_path);
		if (session->ptr) return session->ptr->query(reqq);
		return blueshift::module::mss::proceed;
	}
	
	*processing_token = new router_session {};
	
	if (override_path.size() < mount_head.size()) srcthrow("program error: this should never happen"); 
	session->adj_path = override_path.substr(mount_head.size(), override_path.size() - mount_head.size()); // FIXME? : this assumes the first part of path is equal to mount_head. It shouldn't programmatically ever be different, but it still seems semi-hacky
	blueshift::strops::trim(session->adj_path, '/');
	
	for (auto const & sr : subroutes) {
		if (session->adj_path.size() < sr.first.size()) continue;
		if (sr.first == session->adj_path.substr(0, sr.first.size())) { // the above FIXME? is checked here
			session->subrouter = sr.second;
			return session->subrouter->query(&session->subroute_session, req, reqq, session->adj_path);
		}
	}
	
	auto const & urir = route_table.find(req.method);
	
	if (urir != route_table.end()) {
		route_ptr ptr = urir->second.resolve(session->adj_path);
		if (!ptr) {
			reqq.refuse_payload();
			session->condition = blueshift::http::status_code::not_found;
			return blueshift::module::mss::proceed;
		}
		session->ptr = ptr->create();
		session->ptr->set_request(req);
		session->ptr->adjust_path(session->adj_path);
		return session->ptr->query(reqq);
	}
	
	reqq.refuse_payload();
	session->condition = blueshift::http::status_code::method_not_allowed;
	return blueshift::module::mss::proceed;
}

#undef session
#define session reinterpret_cast<router_session *>(token)

void router::process_begin (void * token, blueshift::http::request_header const & req) {
	if (!session) return;
	if (session->subrouter) return session->subrouter->process_begin(session->subroute_session, req);
	if (session->ptr) return session->ptr->process_begin();
}

blueshift::module::mss router::process (void * token, blueshift::http::request_header const & req, std::vector<char> const & buffer_data) {
	if (!session) return blueshift::module::mss::proceed;
	if (session->subrouter) return session->subrouter->process(session->subroute_session, req, buffer_data);
	if (session->ptr) return session->ptr->process(buffer_data);
	return blueshift::module::mss::proceed;
}

void router::process_end (void * token, blueshift::http::request_header const & req) {
	if (!session) return;
	if (session->subrouter) return session->subrouter->process_end(session->subroute_session, req);
	if (session->ptr) return session->ptr->process_end();
}

void router::process_multipart_begin (void * token, blueshift::http::request_header const & req, blueshift::http::multipart_header const & mh) {
	if (!session) return;
	if (session->subrouter) return session->subrouter->process_multipart_begin(session->subroute_session, req, mh);
	if (session->ptr) return session->ptr->process_multipart_begin(mh);
}

blueshift::module::mss router::process_multipart (void * token, blueshift::http::request_header const & req, blueshift::http::multipart_header const & mh, std::vector<char> const & multipart_data) {
	if (!session) return blueshift::module::mss::proceed;
	if (session->subrouter) return session->subrouter->process_multipart(session->subroute_session, req, mh, multipart_data);
	if (session->ptr) return session->ptr->process_multipart(mh, multipart_data);
	return blueshift::module::mss::proceed;
}

void router::process_multipart_end (void * token, blueshift::http::request_header const & req, blueshift::http::multipart_header const & mh) {
	if (!session) return;
	if (session->subrouter) return session->subrouter->process_multipart_end(session->subroute_session, req, mh);
	if (session->ptr) return session->ptr->process_multipart_end(mh);
}

blueshift::module::mss router::finalize_response (void * token, blueshift::http::request_header const & req, blueshift::http::response_header & res, blueshift::module::response_query & resq) {
	if (!session) {
		blueshift::module::setup_generic_error(res, resq, blueshift::http::status_code::internal_server_error);
		return blueshift::module::mss::proceed;
	}
	if (session->subrouter) {
		return session->subrouter->finalize_response(session->subroute_session, req, res, resq);
		return blueshift::module::mss::proceed;
	}
	if (session->ptr) {
		return session->ptr->finalize_response(res, resq);
		return blueshift::module::mss::proceed;
	}
	blueshift::module::setup_generic_error(res, resq, session->condition);
	return blueshift::module::mss::proceed;
}

void router::cleanup (void * token) {
	if (!session) return;
	delete session;
}

void router::pulse () {
	
}

void router::interface_init () {
	
}

void router::interface_term () {
	
}

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================
