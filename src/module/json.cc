#include "json.hh"
#include "strops.hh"

blueshift::json_data::json_data (blueshift::json_data::type t) : type_ (t) {
	switch (t) {
		case type::nil:
			break;
		case type::nui:
			nui = 0;
			break;
		case type::nuf:
			nuf = 0;
			break;
		case type::str:
			new (&str) std::string();
			break;
		case type::ary:
			new (&ary) std::vector<so>();
			break;
		case type::map:
			new (&map) std::unordered_map<std::string, so>();
			break;
	}
}

blueshift::json_data::json_data (blueshift::json_data const & other) : type_ (other.type_) {
	switch (type_) {
		case type::nil:
			break;
		case type::nui:
			nui = other.nui;
			break;
		case type::nuf:
			nuf = other.nuf;
			break;
		case type::str:
			new (&str) std::string(other.str);
			break;
		case type::ary:
			new (&ary) std::vector<so>(other.ary);
			break;
		case type::map:
			new (&map) std::unordered_map<std::string, so>(other.map);
			break;
	}
}

blueshift::json_data::~json_data () {
	switch (type_) {
		case type::nil:
		case type::nui:
		case type::nuf:
			break;
		case type::str:
			str.std::string::~string();
			break;
		case type::ary:
			ary.~vector();
			break;
		case type::map:
			map.~unordered_map();
			break;
	}
}

blueshift::json_data::so::operator json_integer_t () {
	if (!ptr) return 0;
	switch (ptr->type_) {
		case type::nil:
			return 0;
		case type::nui:
			return ptr->nui;
		case type::nuf:
			return static_cast<json_integer_t>(ptr->nuf);
		case type::str:
			return strtol(ptr->str.c_str(), nullptr, 10);
		case type::ary:
			return 0;
		case type::map:
			return 0;
	}
	return 0;
}

blueshift::json_data::so::operator json_float_t () {
	if (!ptr) return 0;
	switch (ptr->type_) {
		case type::nil:
			return 0;
		case type::nui:
			return static_cast<json_float_t>(ptr->nui);
		case type::nuf:
			return ptr->nuf;
		case type::str:
			return strtof(ptr->str.c_str(), nullptr);
		case type::ary:
			return 0;
		case type::map:
			return 0;
	}
	return 0;
}

blueshift::json_data::so::operator std::string () {
	if (!ptr) return "";
	switch (ptr->type_) {
		case type::nil:
			return "";
		case type::nui:
			return std::to_string(ptr->nui);
		case type::nuf:
			return std::to_string(ptr->nuf);
		case type::str:
			return ptr->str;
		case type::ary:
			return "";
		case type::map:
			return "";
	}
	return "";
}

blueshift::json_data::so & blueshift::json_data::so::operator [] (size_t i) {
	if (!ptr || ptr->type_ != type::ary) {
		ptr.reset(new json_data{type::ary});
	}
	if (ptr->ary.size() <= i) {
		ptr->ary.resize(i);
	}
	return ptr->ary[i];
}

blueshift::json_data::so & blueshift::json_data::so::operator [] (std::string const & key) {
	if (!ptr || ptr->type_ != type::map) {
		ptr.reset(new json_data{type::map});
	}
	return ptr->map[key];
}

blueshift::json_data::so blueshift::json_data::so::get (size_t i) {
	if (!ptr) return {};
	if (ptr->type_ != type::ary) return {};
	if (ptr->ary.size() <= i) return {};
	return ptr->ary[i];
}

blueshift::json_data::so blueshift::json_data::so::get (std::string const & str) {
	if (!ptr) return {};
	if (ptr->type_ != type::map) return {};
	auto const & i = ptr->map.find(str);
	if (i == ptr->map.end()) return {};
	return i->second;
}

// ================================================================

std::string blueshift::json_data::so::serialize () const {
	if (!ptr) return "null";
	switch(ptr->type_) {
		case type::nil:
			break;
		case type::nui:
			return std::to_string(ptr->nui);
		case type::nuf:
			return std::to_string(ptr->nuf);
		case type::str:
			return "\"" + ptr->str + "\"";
		case type::ary: {
			std::string arystr {"["};
			bool first = true;
			for (auto & i : ptr->ary) {
				if (!first) { arystr += ", "; } else first = false;
				arystr += i.serialize();
			}
			arystr += "]";
			return arystr;
		}
		case type::map: {
			std::string arystr {"{"};
			bool first = true;
			for (auto const & i : ptr->map) {
				if (!first) { arystr += ", "; } else first = false;
				arystr += "\"" + i.first + "\": " + i.second.serialize();
			}
			arystr += "}";
			return arystr;
		}
	}
	
	return "null";
}

// ================================================================


typedef std::string::const_iterator sci;

static blueshift::json parse_json_object(sci b, sci e);

static blueshift::json parse_json_num(sci b, sci e) {
	if (b == e) return nullptr;
	
	bool is_float = false;
	for (sci i = b; i != e; i++) {
		if (*i == '.') is_float = true;
	}
	
	if (is_float) {
		return strtof(std::string {b, e}.c_str(), nullptr);
	} else {
		return strtol(std::string {b, e}.c_str(), nullptr, 10);
	}
}

static blueshift::json parse_json_str(sci b, sci e) {
	if (b == e) return nullptr;
	
	std::string str = {b, e};
	blueshift::strops::trim(str, '\"');
	return std::move(str);
}

static blueshift::json parse_json_ary(sci b, sci e) {
	if (std::distance(b, e) < 3) return nullptr;
	
	std::string str = {b + 1, e - 1};
	std::vector<std::string> arysplit;
	
	int braced_section = 0, bracketed_section = 0;
	sci last_i = str.begin();
	for (sci i = str.begin(); i != str.end();) {
		switch (*i) {
			case '[':
				bracketed_section++;
				break;
			case ']':
				bracketed_section--;
				break;
			case '{':
				braced_section++;
				break;
			case '}':
				braced_section--;
				break;
		}
		
		if (braced_section || bracketed_section) {
			i++;
			continue;
		}
		
		if (*i == ',') {
			arysplit.emplace_back(last_i, i);
			i++;
			last_i = i;
			continue;
		}
		
		i++;
	}
	if (last_i < str.end()) arysplit.emplace_back(last_i, (sci)str.end());
	
	blueshift::json obj {blueshift::json_data::type::ary};
	for (std::string const & substr : arysplit) {
		obj->ary.push_back(parse_json_object(substr.begin(), substr.end()));
	}
	return obj;
}

static blueshift::json parse_json_map(sci b, sci e) {
	if (std::distance(b, e) < 3) return nullptr;
	
	std::string str = {b + 1, e - 1};
	std::vector<std::string> mapsplit;
	
	int braced_section = 0, bracketed_section = 0;
	sci last_i = str.begin();
	for (sci i = str.begin(); i != str.end();) {
		switch (*i) {
			case '[':
				bracketed_section++;
				break;
			case ']':
				bracketed_section--;
				break;
			case '{':
				braced_section++;
				break;
			case '}':
				braced_section--;
				break;
		}
		
		if (braced_section || bracketed_section) {
			i++;
			continue;
		}
		
		if (*i == ',') {
			mapsplit.emplace_back(last_i, i);
			i++;
			last_i = i;
			continue;
		}
		
		i++;
	}
	if (last_i < str.end()) mapsplit.emplace_back(last_i, (sci)str.end());
	
	blueshift::json obj {blueshift::json_data::type::map};
	for (std::string const & substr : mapsplit) {
		std::vector<std::string> fieldsplit = blueshift::strops::separate(substr, std::string {":"}, 1);
		blueshift::strops::trim(fieldsplit[0], '\"');
		obj->map[fieldsplit[0]] = parse_json_object(fieldsplit[1].begin(), fieldsplit[1].end());
	}
	return obj;
}

static blueshift::json parse_json_object(sci b, sci e) {
	if (b == e) return nullptr;
	if (*b == '{') return parse_json_map(b, e);
	if (*b == '[') return parse_json_ary(b, e);
	if (*b == '\"') return parse_json_str(b, e);
	if ((*b >= '0' && *b <= '9') || *b == '.' || *b == '-') return parse_json_num(b, e); 
	srcthrow("unexpected symbol during json parsing");
}

blueshift::json_data::so blueshift::json_data::so::parse (std::string src) {
	bool in_quotes = false;
	for (sci i = src.begin(); i != src.end();) {
		if (*i == '\"') in_quotes = !in_quotes;
		else if (!in_quotes) {
			if (*i == ' ' || *i == '\r' || *i == '\n' || *i == '\t') {
				i = src.erase(i); continue;
			}
		}
		i++;
	}
	
	json j = parse_json_object(src.begin(), src.end());
	return j;
}
