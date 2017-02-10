#pragma once

#include "com.hh"

#include <vector>
#include <unordered_map>

namespace blueshift {
	
struct json_data {
	
	typedef long json_integer_t;
	typedef float json_float_t;
	
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
		inline so (json_integer_t i) : ptr(new json_data{type::nui}) { ptr->nui = i; }
		inline so (json_float_t i) : ptr(new json_data{type::nuf}) { ptr->nuf = i; }
		inline so (std::string const & str) : ptr(new json_data{type::str}) { ptr->str = str; }
		inline so (std::string && str) : ptr(new json_data{type::str}) { ptr->str = std::move(str); }
		inline so (char const * str) : ptr(new json_data{type::str}) { ptr->str = str; }
		inline ~so() = default;
		inline so & operator = (json_integer_t i) {
			if (!ptr || ptr->type_ != type::nui) {
				ptr.reset(new json_data {type::nui});
			}
			ptr->nui = i;
			return *this;
		}
		inline so & operator = (json_float_t i) {
			if (!ptr || ptr->type_ != type::nuf) {
				ptr.reset(new json_data {type::nuf});
			}
			ptr->nuf = i;
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
		operator json_integer_t ();
		operator json_float_t ();
		operator std::string ();
		// ----
		so & operator [] (size_t);
		so & operator [] (std::string const &);
		inline so & operator [] (char const * str) {return operator[](std::string{str});}
		// ----
		inline json_data & operator * () { return *ptr; }
		inline json_data const & operator * () const { return *ptr; }
		inline json_data * operator -> () {return ptr.get();}
		inline json_data const * operator -> () const {return ptr.get();}
		inline operator bool () {
			return (ptr && ptr->type_ != type::nil);
		}
		
		//these will not overwrite data on incorrect types, unlike the [] operators
		so get (size_t);
		so get (std::string const &);
		inline so get (char const * str) {return get(std::string{str});}
		// ----
		
		std::string serialize() const;
		static so parse(std::string);
		
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
