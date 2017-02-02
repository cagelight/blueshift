#pragma once

#include "com.hh"
#include "strops.hh"
#include <ctime>

namespace blueshift {
	
	template <clockid_t CLOCK_ID> struct clock {
		
		static constexpr clockid_t clock_id = CLOCK_ID;
		
		static constexpr uint8_t hmask = 0b11110000;
		static constexpr uint8_t lmask = 0b00001111;
		
		struct time_point {
			
			struct timespec data {0, 0};
			
			time_point() = default;
			time_point(struct timespec const & t) : data {t} {}
			time_point(std::string const & etagi) {
				std::string etag = etagi;
				strops::trim(etag, '"');
				
				if (!etag.size()) return;
				
				std::string::const_iterator bi, ei;
				bi = ei = etag.begin();
				for (; ei != etag.end() && *ei != '_'; ei++);
				
				uint8_t * sec_d = reinterpret_cast<uint8_t *>(&data.tv_sec);
				for (size_t i = 0; i < sizeof(data.tv_sec); i++) {
					char vh = *bi;
					char vhs = vh > 57 ? vh - 55 : vh - 48;
					sec_d[i] = vhs << 4;
					if (++bi == ei) break;
					char vl = *bi;
					sec_d[i] |= vl > 57 ? vl - 55 : vl - 48;
					if (++bi == ei) break;
				}
				
				if (ei++ == etag.end() || ei == etag.end()) return;
				bi = ei;
				ei = etag.end();
				
				uint8_t * nsec_d = reinterpret_cast<uint8_t *>(&data.tv_nsec);
				for (size_t i = 0; i < sizeof(data.tv_nsec); i++) {
					char vh = *bi;
					char vhs = vh > 57 ? vh - 55 : vh - 48;
					nsec_d[i] = vhs << 4;
					if (++bi == ei) break;
					char vl = *bi;
					nsec_d[i] |= vl > 57 ? vl - 55 : vl - 48;
					if (++bi == ei) break;
				}
			}
			
			
			
			std::string generate_etag() {
				std::string r {"\""};
				
				uint8_t * sec_d = reinterpret_cast<uint8_t *>(&data.tv_sec);
				size_t sec_dl = sizeof(data.tv_sec);
				
				for (ssize_t i = sec_dl - 1; i >= 0; i--) {
					if (!*(sec_d + i)) sec_dl--;
					else break;
				}
				
				for (size_t i = 0; i < sec_dl; i++) {
					uint8_t cur = *(sec_d + i);
					char vh = (cur & hmask) >> 4;
					char vl = cur & lmask;
					r += vh > 9 ? 55 + vh : 48 + vh;
					r += vl > 9 ? 55 + vl : 48 + vl;
				}
				
				if (data.tv_nsec) {
				
					r += '_';
					
					uint8_t * nsec_d = reinterpret_cast<uint8_t *>(&data.tv_nsec);
					size_t nsec_dl = sizeof(data.tv_nsec);
					
					for (ssize_t i = nsec_dl - 1; i >= 0; i--) {
						if (!*(nsec_d + i)) nsec_dl--;
						else break;
					}
					
					for (size_t i = 0; i < nsec_dl; i++) {
						uint8_t cur = *(nsec_d + i);
						char vh = (cur & hmask) >> 4;
						char vl = cur & lmask;
						r += vh > 9 ? 55 + vh : 48 + vh;
						r += vl > 9 ? 55 + vl : 48 + vl;
					}
				
				}
				
				r += "\"";
				return r;
			}
			
			inline time_point operator - (time_point const & other) {
				struct timespec sum {};
				sum.tv_sec = data.tv_sec - other.data.tv_sec;
				sum.tv_nsec = data.tv_nsec - other.data.tv_nsec;
				while (sum.tv_nsec > 1000000000) {
					sum.tv_nsec -= 1000000000;
					sum.tv_sec ++;
				}
				while (sum.tv_nsec < 0) {
					sum.tv_nsec += 1000000000;
					sum.tv_sec --;
				}
				return sum;
			}
			
			inline bool operator == (time_point const & other) { return data.tv_sec == other.data.tv_sec && data.tv_nsec == other.data.tv_nsec; }
			inline bool operator != (time_point const & other) { return !operator==(other); }
			inline bool operator < (time_point const & other) { 
				if (data.tv_sec < other.data.tv_sec) return true; 
				if (data.tv_sec > other.data.tv_sec) return false; 
				if (data.tv_nsec < other.data.tv_nsec) return true; 
				return false; 
			}
			inline bool operator > (time_point const & other) { 
				if (data.tv_sec > other.data.tv_sec) return true; 
				if (data.tv_sec < other.data.tv_sec) return false; 
				if (data.tv_nsec > other.data.tv_nsec) return true; 
				return false; 
			}
			inline bool operator <= (time_point const & other) { return !operator>(other); }
			inline bool operator >= (time_point const & other) { return !operator<(other); }
		};
		
		static inline time_point now() {
			time_point p;
			clock_gettime(clock_id, &p.data);
			return p;
		}
	};
	
	typedef clock<CLOCK_REALTIME> realtime_clock;
	typedef clock<CLOCK_MONOTONIC> monotonic_clock;
	typedef clock<CLOCK_PROCESS_CPUTIME_ID> process_clock;
	typedef clock<CLOCK_THREAD_CPUTIME_ID> thread_clock;
}
