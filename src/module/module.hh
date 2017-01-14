#pragma once

#include "http.hh"
#include "file.hh"

namespace blueshift {
	
	enum struct module_status : uint8_t {
		complete,
		incomplete,
		waiting,
		terminate,
	};
	
	typedef module_status (*server_request_handler) (http::request const & req, http::response &, void * token);
	
	struct m_import {
		void (*start_server) (uint16_t, server_request_handler);
		void (*stop_server) (uint16_t);
	};
	
}

#define BLUESHIFT_MODULE_INIT_FUNC extern "C" __attribute__((visibility("default"))) void _blueshift_module_init ( blueshift::m_import const * imp )
