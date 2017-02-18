#pragma once

#include "module.hh"

namespace blueshift::pool {
	
	void init ();
	void term () noexcept;
	
	void enter_epoll_loop(); // called from main thread
	
	void start_server(uint16_t, module::interface *);
	void stop_server(uint16_t);
	
}
