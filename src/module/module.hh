#pragma once

#include "http.hh"
#include "file.hh"
#include "time.hh"

namespace blueshift::module {
	
	struct request_query {
		
		void refuse_payload();
		void reset();
		
		// ================

		enum struct qe {
			ok,
			refuse_payload,
		} q = qe::ok;
	};
	
	struct response_query {
		
		void set_body(std::vector<char> const &, char const * MIME = nullptr); // nullptr -> "default" MIME (see blueshift::http::default_mime)
		void set_body(std::vector<char> &&, char const * MIME = nullptr); // nullptr -> "default" MIME (see blueshift::http::default_mime)
		void set_body(shared_file const &, char const * MIME = nullptr); // nullptr -> resolve from extension
		void set_body(std::string const & str, char const * MIME = nullptr); // nullptr -> text/plain
		void set_body(char const * str, char const * MIME = nullptr); // nullptr -> text/plain
		void reset();
		
		// ================
		
		enum struct qe {
			no_body,
			body_buffer,
			body_file,
		} q = qe::no_body;
		std::vector<char> body_buffer;
		shared_file body_file;
		std::string MIME;
	};
	
	enum struct mss : bool { // module subroutine status (mss)
		waiting,
		proceed
	};

	struct interface {
		
		virtual ~interface() = default;
		
		virtual mss query (void * * processing_token, http::request_header const & req, request_query & reqq) = 0;
		
		virtual void process_begin (void * token, http::request_header const & req) = 0;
		virtual mss process (void * token, http::request_header const & req, std::vector<char> const & buffer_data) = 0;
		virtual void process_end (void * token, http::request_header const & req) = 0;
		
		virtual void process_multipart_begin (void * token, http::request_header const & req, http::multipart_header const & mh) = 0;
		virtual mss process_multipart (void * token, http::request_header const & req, http::multipart_header const & mh, std::vector<char> const & multipart_data) = 0;
		virtual void process_multipart_end (void * token, http::request_header const & req, http::multipart_header const & mh) = 0;
		
		virtual mss finalize_response (void * token, http::request_header const & req, http::response_header & res, response_query & resq) = 0;
		
		virtual void cleanup (void * token) = 0;
		
		virtual void pulse () {}
		virtual void interface_init () {}
		virtual void interface_term () {}
		
	protected:
		interface() = default;
	};
	
	struct import {
		void (*start_server) (uint16_t, module::interface *);
		void (*start_server_ssl) (uint16_t, module::interface *, std::string const & key, std::string const & cert);
		void (*stop_server) (uint16_t);
	};
	
}

#define BLUESHIFT_MODULE_INIT_FUNC extern "C" __attribute__((visibility("default"))) void _blueshift_module_init ( blueshift::module::import const * imp )
#define BLUESHIFT_MODULE_TERM_FUNC extern "C" __attribute__((visibility("default"))) void _blueshift_module_term ( )  // OPTIONAL
