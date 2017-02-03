#include "json.hh"

blueshift::json::object::object(blueshift::json::object::type t) : type_ (t) {
	switch (t) {
		case type::none:
			break;
		case type::number:
			number = 0;
			break;
		case type::string:
			new (&string) std::string();
			break;
		case type::array:
			new (&array) std::vector<std::shared_ptr<object>>();
			break;
		case type::map:
			new (&map) std::unordered_map<std::string, std::shared_ptr<object>>();
			break;
	}
}

blueshift::json::object::~object() {
	switch (type_) {
		case type::none:
		case type::number:
			break;
		case type::string:
			string.std::string::~string();
			break;
		case type::array:
			array.~vector();
			break;
		case type::map:
			map.~unordered_map();
			break;
	}
}


std::string blueshift::json::object::serialize() {

	switch(type_) {
		case type::none:
			break;
		case type::number:
			return std::to_string(number);
		case type::string:
			return "\"" + string + "\"";
		case type::array: {
			std::string arystr {"["};
			bool first = true;
			for (auto i : array) {
				if (!first) { arystr += ", "; } else first = false;
				arystr += i->serialize();
			}
			arystr += "]";
			return arystr;
		}
		case type::map: {
			std::string arystr {"{"};
			bool first = true;
			for (auto const & i : map) {
				if (!first) { arystr += ", "; } else first = false;
				arystr += "\"" + i.first + "\": " + i.second->serialize();
			}
			arystr += "}";
			return arystr;
		}
	}
	
	return "";
}


std::string blueshift::json::serialize() {
	if (!root) return "{}";
	return root->serialize();
}
