#pragma once

#include <ctgmath>
#include <vector>
#include <list>

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
	
	template <typename T> bool itereq (T const & a_b, T const & a_e, T const & b_b, T const & b_e) {
		T a_i, b_i;
		for (a_i = a_b, b_i = b_b; a_i != a_e && b_i != b_e; a_i++, b_i++) {
			if (*a_i != *b_i) return false;
		}
		if ((a_i == a_e) && (b_i == b_e)) return true;
		return false;
	}
	
	template <typename T, typename V = std::vector<T>, typename Ti = typename T::const_iterator> V separate (Ti const & a_b, Ti const & a_e, Ti const & b_b, Ti const & b_e, size_t num_sep = SIZE_MAX) {
		V r {};
		if (a_b == a_e || b_b == b_e) return r;
		size_t cur_sep = 0;
		size_t d = std::distance(b_b, b_e);
		Ti i_b = a_b, i_e = a_b + d, i_l = i_b;
		while (i_e != a_e) {
			if (itereq<Ti>(i_b, i_e, b_b, b_e)) {
				if (i_l != i_b) r.push_back( {i_l, i_b} );
				i_b += d;
				i_l = i_b;
				if (++cur_sep == num_sep) goto end;
				for (size_t j = 0; j < d; j++) if (++i_e == a_e) goto end;
				continue;
			}
			i_b++;
			i_e++;
			continue;
		}
		end:
		if (itereq<Ti>(i_b, i_e, b_b, b_e) && i_l != i_b) {
			r.push_back( {i_l, i_b} );
		} else if (i_l != a_e) {
			r.push_back( {i_l, a_e} );
		}
		return r;
	}
	
	template <typename T, typename V = std::vector<T>, typename Ti = typename T::const_iterator> V separate (T const & a, T const & b, size_t num_sep = SIZE_MAX) {
		return separate<T, V, Ti> (a.begin(), a.end(), b.begin(), b.end(), num_sep);
	}
	
	
	template <typename T, typename TT = decltype(T::value_type)> void trim(T & v, TT tv) {
		while (v.size() && *v.begin() == tv) v.erase(v.begin());
		while (v.size() && v.back() == tv) v.erase(v.end() - 1);
	}
	
	static inline bool issymbol1(char c) { return c < 48; }
	static inline bool isnum(char c) { return c > 47 && c < 58; }
	
	template <typename T, typename Ti = typename T::const_iterator> bool natcmp (Ti const & a_b, Ti const & a_e, Ti const & b_b, Ti const & b_e) {
		for (Ti a_i = a_b, b_i = b_b; a_i != a_e && b_i != b_e;) {
			
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
					Ti a_ib = a_i;
					Ti b_ib = b_i;
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
