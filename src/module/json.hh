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
	
	typedef std::shared_ptr<object> so;
	
	static inline so num ( long num = 0 ) { so r { new object {object::type::number} }; r->number = num; return r; }
	static inline so str ( std::string const & str = "" ) { so r { new object {object::type::string} }; r->string = str; return r; }
	static inline so ary ( std::initializer_list<so> init = {} ) { so r { new object {object::type::array} }; r->array = init; return r; }
	static inline so map () { return so { new object {object::type::map} }; }
	
	json() = default;
	~json() = default;
	
	so root;
	
	std::string serialize();
	
	so & operator [] (std::string const & i) { return root->map [i]; }
};

}
