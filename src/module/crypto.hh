#pragma once

#include "com.hh"

namespace blueshift::crypto {
	
	byte_buffer random (size_t len);
	byte_buffer hash_skein (std::string str);
	byte_buffer hash_skein (byte_buffer buf);
	
}
