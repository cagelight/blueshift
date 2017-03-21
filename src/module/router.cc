#include "router.hh"
#include <module_helpers.hh>

#include <set>
#include <unordered_map>
#include <regex>

using namespace blueshift;

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

blueshift::module::mss basic_route::query(blueshift::module::request_query & reqq) {
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

void basic_route::process_multipart_begin (blueshift::http::multipart_header const & mh) {
	multipart mp {mh, {}};
	multiparts.push_back(mp);
}
blueshift::module::mss basic_route::process_multipart (blueshift::http::multipart_header const &, std::vector<char> const & body) { 
	multiparts.back().body.insert(multiparts.back().body.end(), body.begin(), body.end());
	return blueshift::module::mss::proceed;
}
void basic_route::process_multipart_end (blueshift::http::multipart_header const &) {
	
}

blueshift::module::mss basic_route::finalize_response (blueshift::http::response_header & res, blueshift::module::response_query & resq) {
	if (condition != blueshift::http::status_code::ok) {
		blueshift::module::setup_generic_error(res, resq, condition);
		return blueshift::module::mss::proceed;
	}
	response = &res;
	response_query = &resq;
	try {
		respond();
	} catch (blueshift::general_exception & e) {
		srcprintf_warning("basic_route caught a general exception: %s", e.what());
		generic_error(http::status_code::internal_server_error);
	} catch (std::exception & e) {
		srcprintf_warning("basic_route caught an unkown exception (derives from std::exception): %s", e.what());
		generic_error(http::status_code::internal_server_error);
	} catch (...) {
		srcprintf_warning("basic_route caught an unknown exception that it could not understand.");
		generic_error(http::status_code::internal_server_error);
	}
	return blueshift::module::mss::proceed;
}

void blueshift::basic_route::generic_error(http::status_code code, std::string const & admsg) {
	module::setup_generic_error(*response, *response_query, code, admsg);
}

bool blueshift::basic_route::serve_file(std::string const & file_root, bool directory_listing, bool htmlcheck) {
	return module::serve_static_file(*request, *response, *response_query, path_head, path, file_root, directory_listing, htmlcheck);
}

bool blueshift::basic_route::serve_file(std::string const & path_head, std::string const & path_tail, std::string const & file_root, bool directory_listing, bool htmlcheck) {
	return module::serve_static_file(*request, *response, *response_query, path_head, path_tail, file_root, directory_listing, htmlcheck);
}

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

	/*
	
bool router::uri_resolver::regex_route::search(std::string const & path) const {
	return std::regex_search(path, reg);
}
		struct regex_route {
			std::string str;
			std::regex reg;
			route_ptr reb = nullptr;
			regex_route(std::string str, route_ptr ptr) : str(str), reg(str, std::regex_constants::icase | std::regex_constants::optimize | std::regex_constants::extended), reb(ptr) {}
			bool search (std::string const & path) const;
			struct hash { uint64_t operator() (regex_route const & rt) const { return std::hash<std::string>{}(rt.str); } };
			struct comp { bool operator() (regex_route const & A, regex_route const & B) const { return A.str < B.str; } };
		};

	*/
	
struct router_data {
	
	struct root_route {
		std::string str;
		root_route(std::string const & root) : str(root) {}
		bool is_root_of (std::string const & path) const {
			if (str.size() > path.size()) return false;
			return !strncmp(str.c_str(), path.c_str(), str.size());
		}
	};
	
	struct regex_route {
		std::string str;
		std::regex reg;
		router::route_ptr reb = nullptr;
		regex_route (std::string str, router::route_ptr ptr) : str(str), reg(str, std::regex_constants::icase | std::regex_constants::optimize | std::regex_constants::extended), reb(ptr) {}
		bool search (std::string const & path) const;
		struct hash { uint64_t operator() (regex_route const & rt) const { return std::hash<std::string>{}(rt.str); } };
		struct comp { bool operator() (regex_route const & A, regex_route const & B) const { return A.str < B.str; } };
	};
	
	std::unordered_map<std::string, std::unordered_map<std::string, router::route_ptr>> exact_routes {};
	
	typedef std::pair<root_route, std::unordered_map<std::string, router::route_ptr>> root_pair;
	struct root_pair_comp { bool operator() (root_pair const & A, root_pair const & B) const { return A.first.str < B.first.str; } };
	std::vector<root_pair> root_routes {};
	
	std::unordered_map<std::string, router::route_ptr> fallback_routes {};
};

#define rd reinterpret_cast<::router_data *>(this->router_data)

void blueshift::router::route_exact_insert(std::string path, route_ptr rt, std::vector<std::string> const & methods) {
	for (auto const & m : methods) {
		rd->exact_routes[path][m] = rt;
	}
}

void blueshift::router::route_root_insert(std::string path_root, route_ptr rt, std::vector<std::string> const & methods) {
	router_data::root_pair * r = nullptr;
	for (auto & rp : rd->root_routes) {
		if (rp.first.str == path_root) {
			r = &rp;
			break;
		}
	}
	if (!r) {
		rd->root_routes.push_back({path_root, {}});
		r = &rd->root_routes.back();
		for (auto const & m : methods) {
			r->second[m] = rt;
		}
		std::sort(rd->root_routes.begin(), rd->root_routes.end(), router_data::root_pair_comp());
	} else for (auto const & m : methods) {
		r->second[m] = rt;
	}
}

void blueshift::router::route_regex_insert(std::string path_regex, route_ptr rt, std::vector<std::string> const & methods) {
	// TODO
}

void blueshift::router::route_fallback_set(route_ptr rt, std::vector<std::string> const & methods) {
	for (auto const & m : methods) {
		rd->fallback_routes[m] = rt;
	}
}

blueshift::router::router() {
	this->router_data = new ::router_data {};
}

blueshift::router::~router() {
	if (rd) delete rd;
}

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
	
	auto const & exact_match = rd->exact_routes.find(session->adj_path);
	if (exact_match != rd->exact_routes.end()) {
		auto const & r = exact_match->second.find(req.method);
		if (r == exact_match->second.end()) {
			session->condition = blueshift::http::status_code::method_not_allowed;
			return blueshift::module::mss::proceed;
		} else {
			session->ptr = r->second->create();
			session->ptr->set_request(req);
			std::string head = req.path.substr(0, req.path.size() - session->adj_path.size());
			strops::trim(head, '/');
			session->ptr->adjust_path(head, session->adj_path);
			return session->ptr->query(reqq);
		}
	}
	
	router_data::root_pair const * root_match = nullptr;
	for (auto const & rp : rd->root_routes) {
		if (rp.first.is_root_of(session->adj_path)) {
			root_match = &rp;
			break;
		}
	}
	if (root_match != nullptr) {
		auto const & r = root_match->second.find(req.method);
		if (r == root_match->second.end()) {
			session->condition = blueshift::http::status_code::method_not_allowed;
			return blueshift::module::mss::proceed;
		} else {
			session->ptr = r->second->create();
			session->ptr->set_request(req);
			std::string rootless = session->adj_path.substr(root_match->first.str.size());
			strops::trim(rootless, '/');
			std::string head = req.path.substr(0, req.path.size() - rootless.size());
			strops::trim(head, '/');
			session->ptr->adjust_path(head, rootless);
			return session->ptr->query(reqq);
		}
	}
	
	auto const & fallback = rd->fallback_routes.find(req.method);
	if (fallback != rd->fallback_routes.end()) {
		session->ptr = fallback->second->create();
		session->ptr->set_request(req);
		std::string head = req.path.substr(0, req.path.size() - session->adj_path.size());
		strops::trim(head, '/');
		session->ptr->adjust_path(head, session->adj_path);
		return session->ptr->query(reqq);
	}
	
	reqq.refuse_payload();
	session->condition = blueshift::http::status_code::not_found;
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
	for (auto f : pulsefuncs) f();
}

void router::interface_init () {
	
}

void router::interface_term () {
	
}

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================
