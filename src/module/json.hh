#pragma once

#include "com.hh"

#include <vector>
#include <unordered_map>

namespace blueshift {
	
struct json {
	
	struct object;
	
	struct shared_json_ptr {
		shared_json_ptr(object * obj) : ptr {obj} { }
		std::shared_ptr<object> ptr;
		shared_json_ptr() = default;
		~shared_json_ptr() = default;
		object * operator -> () const {return ptr.operator->();}
		shared_json_ptr & operator = (shared_json_ptr const & other) = default;
		shared_json_ptr & operator [] (std::string const & i) { return ptr->map [i]; }
	};
	
	struct object {
		const enum struct type : uint8_t {
			none,
			number,
			string,
			array,
			map,
		} type_;
		object() = delete;
		object(type t);
		~object();
		
		std::string serialize();
		
		union {
			long number;
			std::string string;
			std::vector<shared_json_ptr> array;
			std::unordered_map<std::string, shared_json_ptr> map;
		};
		
		shared_json_ptr & operator [] (std::string const & i) { return map [i]; }
	};
	
	static inline shared_json_ptr num ( long num = 0 ) { shared_json_ptr r { new object {object::type::number} }; r->number = num; return r; }
	static inline shared_json_ptr str ( std::string const & str = "" ) { shared_json_ptr r { new object {object::type::string} }; r->string = str; return r; }
	static inline shared_json_ptr ary ( std::initializer_list<shared_json_ptr> init = {} ) { shared_json_ptr r { new object {object::type::array} }; r->array = init; return r; }
	static inline shared_json_ptr map () { return shared_json_ptr { new object {object::type::map} }; }
	
	json() = default;
	~json() = default;
	
	object root {object::type::map};
	
	std::string serialize();
	
	shared_json_ptr & operator [] (std::string const & i) { return root.map [i]; }
};

}
