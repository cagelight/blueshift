#pragma once

#include <module.hh>
#include <unordered_map>
#include <set>
#include <memory>
#include <vector>
#include <unordered_set>

namespace blueshift {

struct route { friend struct router;
	virtual ~route() = default;
	void set_request(blueshift::http::request_header const & req) {request = &req;}
	void adjust_path(std::string const & new_path_head, std::string const & new_path) {path_head = new_path_head, path = new_path;}
protected:
	route() = default;
	blueshift::http::request_header const * request = nullptr;
	std::string path_head {""};
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

struct basic_route : public route {
	basic_route() = default;
	virtual ~basic_route() = default;
	virtual void respond() = 0;
protected:
	struct multipart {
		http::multipart_header header;
		std::vector<char> body;
	};
	size_t max_content_size = 52428800; // 50 MiB
	blueshift::http::response_header * response;
	blueshift::module::response_query * response_query;
	std::vector<char> body;
	std::vector<multipart> multiparts;
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

struct static_serve : public basic_route {
	std::string const & file_root;
	static_serve(std::string const & fr) : file_root (fr) {}
	void respond() {
		if (this->serve_file(file_root)) return;
		generic_error(http::status_code::not_found);
	}
};

struct router : public blueshift::module::interface {
	
	router();
	virtual ~router();
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
	struct route_endpoint_static_serve : public route_endpoint_base {
		std::string file_root;
		route_endpoint_static_serve(std::string file_root) : file_root(file_root) {}
		virtual blueshift::route * create() {
			static_serve * ss = new static_serve(file_root);
			return ss;
		}
	};
	template <typename T> struct route_endpoint : public route_endpoint_base {
		virtual blueshift::route * create() {
			return new T {};
		}
	};
	typedef std::shared_ptr<route_endpoint_base> route_ptr;
	
	std::shared_ptr<router> route_route(std::string mount_point) {
		std::shared_ptr<router> rt {new router {}};
		rt->mount_head = mount_point;
		subroutes[mount_point] = rt;
		return rt;
	}
	template <typename T> std::shared_ptr<route_endpoint<T>> route_exact(std::string path, std::initializer_list<std::string> methods) {
		static_assert(std::is_base_of<blueshift::route, T>::value, "T must be derive from route");
		blueshift::strops::trim(path, '/');
		std::shared_ptr<route_endpoint<T>> ptr {new route_endpoint<T> {}};
		route_exact_insert(path, ptr, methods);
		return ptr;
	}
	template <typename T> std::shared_ptr<route_endpoint<T>> route_root(std::string path_root, std::initializer_list<std::string> methods) {
		blueshift::strops::trim(path_root, '/');
		static_assert(std::is_base_of<blueshift::route, T>::value, "T must be derive from route");
		std::shared_ptr<route_endpoint<T>> ptr {new route_endpoint<T> {}};
		route_root_insert(path_root, ptr, methods);
		return ptr;
	}
	template <typename T> std::shared_ptr<route_endpoint<T>> route_regex(std::string path_regex, std::initializer_list<std::string> methods) {
		static_assert(std::is_base_of<blueshift::route, T>::value, "T must be derive from route");
		std::shared_ptr<route_endpoint<T>> ptr {new route_endpoint<T> {}};
		route_regex_insert(path_regex, ptr, methods);
		return ptr;
	}
	template <typename T> std::shared_ptr<route_endpoint<T>> route_fallback(std::initializer_list<std::string> methods) {
		static_assert(std::is_base_of<blueshift::route, T>::value, "T must be derive from route");
		std::shared_ptr<route_endpoint<T>> ptr {new route_endpoint<T> {}};
		route_fallback_set(ptr, methods);
		return ptr;
	}
	void route_serve(std::string path_root, std::string file_root) {
		blueshift::strops::trim(path_root, '/');
		if (path_root == empty_str) route_fallback_set(std::shared_ptr<route_endpoint_static_serve> {new route_endpoint_static_serve{file_root}}, {"GET"});
		else route_root_insert(path_root, std::shared_ptr<route_endpoint_static_serve> {new route_endpoint_static_serve{file_root}}, {"GET"});
	}
	
private:
	
	void * router_data = nullptr;
	std::string mount_head {""};
	std::unordered_map<std::string, std::shared_ptr<router>> subroutes {};
	
	void route_exact_insert(std::string path, route_ptr rt, std::vector<std::string> const & methods);
	void route_root_insert(std::string path_root, route_ptr rt, std::vector<std::string> const & methods);
	void route_regex_insert(std::string path_regex, route_ptr rt, std::vector<std::string> const & methods);
	void route_fallback_set(route_ptr rt, std::vector<std::string> const & methods);
};

}
