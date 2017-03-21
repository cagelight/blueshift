#pragma once

#include "module.hh"

namespace blueshift::pool {
	
	void init ();
	void term () noexcept;
	
	void enter_epoll_loop(); // called from main thread
	
	void start_server(uint16_t, module::interface *);
	void start_server_ssl(uint16_t, module::interface *, std::string const & key, std::string const & cert);
	void stop_server(uint16_t);
	
}
