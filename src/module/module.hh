#pragma once

#include "http.hh"
#include "file.hh"

namespace blueshift::module {
	
	struct request_query {
		
		void set_token(void * token);
		void refuse_payload();
		void use_proxy_resolve(char const * host, char const * service);
		void use_proxy_direct(char const * ip_addr, uint16_t port);
		void reset();
		
		// ================

		enum struct qe {
			ok,
			refuse_payload,
			proxy_direct,
			proxy_resolve,
		} q = qe::ok;
		std::string proxy_host;
		std::string proxy_service;
		uint16_t proxy_port;
		void * processing_token;
	};
	
	struct response_query {
		
		void set_body(std::vector<char> const &, char const * MIME);
		void set_body(std::vector<char> &&, char const * MIME);
		void set_body(shared_file const &, char const * MIME = nullptr); // nullptr -> resolve from extension
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
		mss (*query) (http::request_header const & req, request_query & reqq);
		mss (*process) (void * token, std::vector<char> const & buffer_data);
		mss (*finalize_response) (void * token, http::response_header & res, response_query & resq);
		void (*cleanup) (void * token);
	};
	
	struct import {
		void (*start_server) (uint16_t, module::interface);
		void (*stop_server) (uint16_t);
	};
	
}

#define BLUESHIFT_MODULE_INIT_FUNC extern "C" __attribute__((visibility("default"))) void _blueshift_module_init ( blueshift::module::import const * imp )
