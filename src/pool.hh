#pragma once

#include "module.hh"

namespace blueshift::pool {
	
	void init ();
	void term () noexcept;
	
	void start_server(uint16_t, module::interface *);
	void stop_server(uint16_t);
	
}
