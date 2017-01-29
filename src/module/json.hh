#pragma once

#include "com.hh"

#include <vector>
#include <unordered_map>

namespace blueshift {
	
struct json {

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
			std::vector<std::shared_ptr<object>> array;
			std::unordered_map<std::string, std::shared_ptr<object>> map;
		};
		
		std::shared_ptr<object> & operator [] (std::string const & i) { return map [i]; }
	};
	
	static inline std::shared_ptr<object> num ( long num = 0 ) { std::shared_ptr<object> r { new object {object::type::number} }; r->number = num; return r; }
	static inline std::shared_ptr<object> str ( std::string const & str = "" ) { std::shared_ptr<object> r { new object {object::type::string} }; r->string = str; return r; }
	static inline std::shared_ptr<object> ary ( std::initializer_list<std::shared_ptr<object>> init = {} ) { std::shared_ptr<object> r { new object {object::type::array} }; r->array = init; return r; }
	static inline std::shared_ptr<object> map () { return std::shared_ptr<object> { new object {object::type::map} }; }
	
	json() = default;
	~json() = default;
	
	object root {object::type::map};
	
	std::string serialize();
	
	std::shared_ptr<object> & operator [] (std::string const & i) { return root.map [i]; }
};

}
