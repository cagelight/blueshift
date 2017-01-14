#pragma once

#include "module.hh"

namespace blueshift::pool {
	
	void init ();
	void term () noexcept;
	
	void start_server(uint16_t, server_request_handler);
	void stop_server(uint16_t);
	
}
