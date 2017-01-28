#pragma once

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
	
}
