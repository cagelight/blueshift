#pragma once

#include <module.hh>
#include <unordered_map>
#include <set>
#include <regex>
#include <memory>
#include <vector>
#include <unordered_set>

namespace blueshift {

struct route { friend struct router;
	virtual ~route() = default;
	void set_request(blueshift::http::request_header const & req) {request = &req;}
	void adjust_path(std::string const & new_path) {path = new_path;}
protected:
	route() = default;
	blueshift::http::request_header const * request = nullptr;
	std::string path {""};
	virtual blueshift::module::mss query(blueshift::module::request_query & reqq) = 0;
	virtual void process_begin () = 0;
	virtual blueshift::module::mss process (std::vector<char> const & buffer_data) = 0;
	virtual void process_end () = 0;
	virtual void process_multipart_begin (blueshift::http::multipart_header const & mh) = 0;
	virtual blueshift::module::mss process_multipart (blueshift::http::multipart_header const & mh, std::vector<char> const & multipart_data) = 0;
	virtual void process_multipart_end (blueshift::http::multipart_header const & mh) = 0;
	virtual blueshift::module::mss finalize_response (blueshift::http::response_header & res, blueshift::module::response_query & resq) = 0;
};

struct basic_route : public route { // NO MULTIPART
	basic_route() = default;
	virtual ~basic_route() = default;
	virtual void respond() = 0;
protected:
	size_t max_content_size = 52428800; // 50 MiB
	blueshift::http::response_header * response;
	blueshift::module::response_query * response_query;
	std::vector<char> body;
	void generic_error(http::status_code code);
	bool serve_file(std::string const & file_root, bool directory_listing = false, bool htmlcheck = true);
	bool serve_file(std::string const & path_head, std::string const & path_tail, std::string const & file_root, bool directory_listing = false, bool htmlcheck = true);
private:
	blueshift::http::status_code condition = blueshift::http::status_code::ok;
	virtual blueshift::module::mss query(blueshift::module::request_query & reqq);
	virtual void process_begin ();
	virtual blueshift::module::mss process (std::vector<char> const & buffer_data);
	virtual void process_end ();
	virtual void process_multipart_begin (blueshift::http::multipart_header const & mh);
	virtual blueshift::module::mss process_multipart (blueshift::http::multipart_header const & mh, std::vector<char> const & multipart_data);
	virtual void process_multipart_end (blueshift::http::multipart_header const & mh);
	virtual blueshift::module::mss finalize_response (blueshift::http::response_header & res, blueshift::module::response_query & resq);
};

struct router : public blueshift::module::interface {
	
	router() : interface() {}
	virtual ~router() = default;
	blueshift::module::mss query (void * * processing_token, blueshift::http::request_header const & req, blueshift::module::request_query & reqq, std::string override_path);
	virtual blueshift::module::mss query (void * * processing_token, blueshift::http::request_header const & req, blueshift::module::request_query & reqq) {return query(processing_token, req, reqq, req.path);}
	virtual void process_begin (void * token, blueshift::http::request_header const & req);
	virtual blueshift::module::mss process (void * token, blueshift::http::request_header const & req, std::vector<char> const & buffer_data);
	virtual void process_end (void * token, blueshift::http::request_header const & req);
	virtual void process_multipart_begin (void * token, blueshift::http::request_header const & req, blueshift::http::multipart_header const & mh);
	virtual blueshift::module::mss process_multipart (void * token, blueshift::http::request_header const & req, blueshift::http::multipart_header const & mh, std::vector<char> const & multipart_data);
	virtual void process_multipart_end (void * token, blueshift::http::request_header const & req, blueshift::http::multipart_header const & mh);
	virtual blueshift::module::mss finalize_response (void * token, blueshift::http::request_header const & req, blueshift::http::response_header & res, blueshift::module::response_query & resq);
	virtual void cleanup (void * token);
	virtual void pulse ();
	virtual void interface_init ();
	virtual void interface_term ();
	
	struct route_endpoint_base {
		virtual blueshift::route * create() = 0;
	};
	template <typename T> struct route_endpoint : public route_endpoint_base {
		virtual blueshift::route * create() {
			return new T {};
		}
	};
	typedef std::shared_ptr<route_endpoint_base> route_ptr;
	
	std::shared_ptr<router> route_route(std::string mount_point) {
		std::shared_ptr<router> rt {new router};
		rt->mount_head = mount_point;
		subroutes[mount_point] = rt;
		return rt;
	}
	template <typename T> std::shared_ptr<route_endpoint<T>> route_exact(std::string path, std::initializer_list<std::string> methods) {
		static_assert(std::is_base_of<blueshift::route, T>::value, "T must be derive from route");
		std::shared_ptr<route_endpoint<T>> ptr {new route_endpoint<T> {}};
		for (auto const & i : methods) {
			route_table[i].exact_matches.insert({path, ptr});
		}
		return ptr;
	}
	template <typename T> std::shared_ptr<route_endpoint<T>> route_root(std::string path_root, std::initializer_list<std::string> methods) {
		static_assert(std::is_base_of<blueshift::route, T>::value, "T must be derive from route");
		std::shared_ptr<route_endpoint<T>> ptr {new route_endpoint<T> {}};
		for (auto const & i : methods) {
			route_table[i].root_matches.insert({path_root, ptr});
		}
		return ptr;
	}
	template <typename T> std::shared_ptr<route_endpoint<T>> route_regex(std::string path_regex, std::initializer_list<std::string> methods) {
		static_assert(std::is_base_of<blueshift::route, T>::value, "T must be derive from route");
		std::shared_ptr<route_endpoint<T>> ptr {new route_endpoint<T> {}};
		for (auto const & i : methods) {
			route_table[i].regex_matches.insert({path_regex, ptr});
		}
		return ptr;
	}
	template <typename T> std::shared_ptr<route_endpoint<T>> route_fallback(std::initializer_list<std::string> methods) {
		static_assert(std::is_base_of<blueshift::route, T>::value, "T must be derive from route");
		std::shared_ptr<route_endpoint<T>> ptr {new route_endpoint<T> {}};
		for (auto const & i : methods) {
			route_table[i].fallback = ptr;
		}
		return ptr;
	}
	
private:
	std::string mount_head {""};
	struct uri_resolver {
		struct exact_route {
			std::string str;
			route_ptr reb = nullptr;
			exact_route(std::string str, route_ptr ptr) : str(str), reb(ptr) {}
			struct hash { uint64_t operator() (exact_route const & rt) const { return std::hash<std::string>{}(rt.str); } };
			struct comp { bool operator() (exact_route const & A, exact_route const & B) const { return A.str < B.str; } };
		};
		struct regex_route {
			std::string str;
			std::regex reg;
			route_ptr reb = nullptr;
			regex_route(std::string str, route_ptr ptr) : str(str), reg(str, std::regex_constants::icase | std::regex_constants::optimize | std::regex_constants::extended), reb(ptr) {}
			bool search (std::string const & path) const;
			struct hash { uint64_t operator() (regex_route const & rt) const { return std::hash<std::string>{}(rt.str); } };
			struct comp { bool operator() (regex_route const & A, regex_route const & B) const { return A.str < B.str; } };
		};
		std::unordered_set<exact_route, exact_route::hash, exact_route::comp> exact_matches {};
		std::unordered_set<exact_route, exact_route::hash, exact_route::comp> root_matches {};
		std::unordered_set<regex_route, regex_route::hash, regex_route::comp> regex_matches {};
		route_ptr fallback = nullptr;
		route_ptr resolve(std::string const & path) {
			for (auto const & i : exact_matches)
				if (path == i.str) return i.reb;
			for (auto const & i : root_matches)
				if (path.size() >= i.str.size() && path.substr(0, i.str.size()) == i.str) return i.reb;
			for (auto const & i : regex_matches)
				if (i.search(path)) return i.reb;
			return fallback;
		}
	};
	std::unordered_map<std::string, std::shared_ptr<router>> subroutes {};
	std::unordered_map<std::string, uri_resolver> route_table {};
};

}
