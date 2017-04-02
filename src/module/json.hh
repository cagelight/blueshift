#pragma once

#include "com.hh"

#include <vector>
#include <unordered_map>

namespace blueshift {
	
struct json_data {
	
	typedef long json_integer_t;
	typedef double json_float_t;
	
	const enum struct type : uint8_t {
		nil,
		nui,
		nuf,
		str,
		ary,
		map,
	} type_ = type::nil;
	
	json_data() = delete;
	json_data(type t);
	json_data(json_data const & other);
	~json_data();
	
	struct so {
		std::shared_ptr<json_data> ptr = nullptr;
		inline so () = default;
		inline so (so const & other) = default;
		inline so (type t) : ptr(new json_data{t}) {}
		template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0> inline so (T t) : ptr(new json_data{type::nui}) {ptr->nui = t;}
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0> inline so (T t) : ptr(new json_data{type::nuf}) {ptr->nuf = t;}
		inline so (std::string const & str) : ptr(new json_data{type::str}) { ptr->str = str; }
		inline so (std::string && str) : ptr(new json_data{type::str}) { ptr->str = std::move(str); }
		inline so (char const * str) : ptr(new json_data{type::str}) { ptr->str = str; }
		inline ~so() = default;
		
		inline bool is_nui() const { return ptr && ptr->type_ == type::nui; }
		inline bool is_nuf() const { return ptr && ptr->type_ == type::nuf; }
		inline bool is_str() const { return ptr && ptr->type_ == type::str; }
		inline bool is_ary() const { return ptr && ptr->type_ == type::ary; }
		inline bool is_map() const { return ptr && ptr->type_ == type::map; }
		
		json_integer_t to_int() const;
		json_float_t to_float() const;
		std::string to_string() const;
		
		template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0> inline so & operator = (T t) {
			if (!ptr || ptr->type_ != type::nui) {
				ptr.reset(new json_data {type::nui});
			}
			ptr->nui = t;
			return *this;
		}
		
		template <typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0> inline so & operator = (T t) {
			if (!ptr || ptr->type_ != type::nuf) {
				ptr.reset(new json_data {type::nuf});
			}
			ptr->nuf = t;
			return *this;
		}
		
		inline so & operator = (std::string const & str) {
			if (!ptr || ptr->type_ != type::str) {
				ptr.reset(new json_data {type::str});
			}
			ptr->str = str;
			return *this;
		}
		inline so & operator = (std::string && str) {
			if (!ptr || ptr->type_ != type::str) {
				ptr.reset(new json_data {type::str});
			}
			ptr->str = std::move(str);
			return *this;
		}
		inline so & operator = (char const * str) {
			if (!ptr || ptr->type_ != type::str) {
				ptr.reset(new json_data {type::str});
			}
			ptr->str = str;
			return *this;
		}
		inline so & operator = (so const & other) {
			ptr = other.ptr;
			return *this;
		}
		// ----
		inline operator json_integer_t () const { return to_int(); }
		inline operator json_float_t () const { return to_float(); }
		inline operator std::string () const { return to_string(); }
		// ----
		so & operator [] (size_t);
		so & operator [] (std::string const &);
		inline so & operator [] (char const * str) {return operator[](std::string{str});}
		// ----
		inline json_data & operator * () { return *ptr; }
		inline json_data const & operator * () const { return *ptr; }
		inline json_data * operator -> () {return ptr.get();}
		inline json_data const * operator -> () const {return ptr.get();}
		inline operator bool () const { return (ptr && ptr->type_ != type::nil); }
		
		//these will not overwrite data on incorrect types, unlike the [] operators
		so get (size_t) const;
		so get (std::string const &) const;
		// ----
		
		std::string serialize() const;
		static so parse(std::string);
		
		static inline so nui(json_integer_t i) { so v = {type::nui}; v->nui = i; return v; }
		static inline so nui(std::string const & i) { so v = {type::nui}; v->nui = strtol(i.c_str(), nullptr, 10); return v; }
		static inline so nuf(json_float_t f) { so v = {type::nuf}; v->nuf = f; return v; }
		static inline so nuf(std::string const & f) { so v = {type::nuf}; v->nuf = strtod(f.c_str(), nullptr); return v; }
		static inline so ary() { return {type::ary}; }
		static inline so map() { return {type::map}; }
	};
	
	union {
		json_integer_t nui;
		json_float_t nuf;
		std::string str;
		std::vector<so> ary;
		std::unordered_map<std::string, so> map;
	};
};

typedef json_data::so json;

}
