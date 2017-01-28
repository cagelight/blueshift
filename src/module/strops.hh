#pragma once

#include <ctgmath>

namespace blueshift::strops {
	
	template <typename T, typename TT = decltype(T::value_type)> void remove_duplicates(T & v, TT tv) {
		if (!v.size()) return;
		TT lv = *v.begin();
		for (typename T::iterator i = v.begin() + 1; i != v.end();) {
			if (lv == *i && lv == tv) {
				i = v.erase(i);
				continue;
			}
			lv = *i;
			i++;
		}
	}
	
	template <typename T, typename TT = decltype(T::value_type)> void trim(T & v, TT tv) {
		while (v.size() && *v.begin() == tv) v.erase(v.begin());
		while (v.size() && v.back() == tv) v.erase(v.end() - 1);
	}
	
	static inline bool issymbol1(char c) { return c < 48; }
	static inline bool isnum(char c) { return c > 47 && c < 58; }
	static inline int tonum(char c) { return c - 48; }
	
	template <typename T> bool natcmp (T const & a_b, T const & a_e, T const & b_b, T const & b_e) {
		for (T a_i = a_b, b_i = b_b; a_i != a_e && b_i != b_e;) {
			
			if (issymbol1(*a_i)) {
				if (issymbol1(*b_i)) {
					if (*a_i < *b_i) return true;
					if (*a_i > *b_i) return false;
				} else return true;
			} else if (issymbol1(*b_i)) {
				return false;
			}
			
			if (isnum(*a_i)) {
				if (isnum(*b_i)) {
					T a_ib = a_i;
					T b_ib = b_i;
					for(;a_i != a_e && isnum(*a_i); a_i++);
					for(;b_i != b_e && isnum(*b_i); b_i++);
					unsigned long num_a = strtoul(std::string{a_ib, a_i}.c_str(), nullptr, 10);
					unsigned long num_b = strtoul(std::string{b_ib, b_i}.c_str(), nullptr, 10);
					if (num_a < num_b) return true;
					else if (num_a > num_b) return false;
					else continue;
				} else return true;
			} else if (isnum(*b_i)) {
				return false;
			}
			
			if (*a_i < *b_i) return true;
			if (*a_i > *b_i) return false;
			a_i++, b_i++;
			continue;
		}
		return false;
	}
	
}
