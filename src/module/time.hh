#pragma once

#include "com.hh"

#include <ctime>

namespace blueshift::time {
	
	struct point {
		
		point() = default;
		point(struct timespec const & t) : data {t} {}
		point(std::string const & etag);
		
		struct timespec data {0, 0};
		
		std::string generate_etag();
		
		inline bool operator == (point const & other) { return data.tv_sec == other.data.tv_sec && data.tv_nsec == other.data.tv_nsec; }
		inline bool operator != (point const & other) { return !operator==(other); }
		inline bool operator < (point const & other) { 
			if (data.tv_sec < other.data.tv_sec) return true; 
			if (data.tv_sec > other.data.tv_sec) return false; 
			if (data.tv_nsec < other.data.tv_nsec) return true; 
			return false; 
		}
		inline bool operator > (point const & other) { 
			if (data.tv_sec > other.data.tv_sec) return true; 
			if (data.tv_sec < other.data.tv_sec) return false; 
			if (data.tv_nsec > other.data.tv_nsec) return true; 
			return false; 
		}
		inline bool operator <= (point const & other) { return !operator>(other); }
		inline bool operator >= (point const & other) { return !operator<(other); }
	};
	
}
