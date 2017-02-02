#pragma once

#include <list>
#include <functional>

template <typename T> struct opper {
	
	typedef typename std::list<T>::iterator iterator;
	
	std::list<T> values;
	opper() = default;
	
	operator std::list<T> && () {return std::move(values);}
	
	iterator begin() { return values.begin(); }
	iterator end() { return values.end(); }
	
	size_t count () { return values.size(); }
	size_t count_where (std::function<bool (T&)> func) {
		size_t c;
		for (T & v : values) {
			if (func(v)) c++;
		}
		return c;
	}
	
	template <typename X = T> X join (std::function<X (T &)> func) {
		X x {};
		for (T & t : values) {
			x += func(t);
		}
		return x;
	}
	
	template <typename X = T> X join_with (std::function<X (T &)> func, X inter) {
		X x {};
		bool first = true;
		for (T & t : values) {
			if (!first) x += inter;
			first  = false;
			x += func(t);
		}
		return x;
	}
	
	template <typename X> opper<X> select (std::function<X (T &)> func) {
		opper<X> opr;
		for (T & v : values) {
			opr.values.push_back(func(v));
		}
		return opr;
	}
	
	template <typename X> opper<X> select_where (std::function<X (T &)> func_s, std::function<bool (T&)> func_w) {
		opper<X> opr;
		for (T & v : values) {
			if (func_w(v)) opr.values.push_back(func_s(v));
		}
		return opr;
	}
	
	template <typename Xi, typename X = typename std::remove_reference<Xi>::type::value_type> opper<X> select_many (std::function<Xi (T &)> func) {
		opper<X> opr;
		for (T & v : values) {
			Xi vs = func(v);
			for (X & x : vs) {
				opr.values.push_back(x);
			}
		}
		return opr;
	}
	
	opper<T> & where(std::function<bool (T&)> func) {
		iterator li = values.begin();
		while (li != values.end()) {
			if (func(*li)) {
				li++;
				continue;
			} else {
				li = values.erase(li);
				continue;
			}
		}
		return *this;
	}
};

template <typename T> struct opper_s {
	typedef typename T::value_type Tt;
	T & ref;
	opper_s () = delete;
	opper_s (T & t) : ref(t) {}
	
	opper<Tt *> where(std::function<bool (Tt &)> func) {
		opper<Tt *> opr;
		for (auto & i : ref) {
			if (func(i)) opr.values.push_back(&i);
		}
		return opr;
	}
};
