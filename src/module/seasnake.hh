#pragma once

#include <type_traits>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace seasnake {
	
// ================================================================
	
	typedef uint64_t size_type;
	typedef uint16_t string_size_type;

	struct serializer {
		
		std::ofstream stream;
		
		serializer() = delete;
		serializer(std::string const & path) : stream(path, std::ios::binary) {}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value && !std::is_array<T>::value, int>::type = 0> void serialize (T const & value) {
			stream.write(reinterpret_cast<char const *>(&value), sizeof(T));
		}
		
		template <typename T, typename std::enable_if<std::is_same<typename T::is_seasnake, std::true_type>::value, int>::type = 0> void serialize (T const & value) {
			value.serialize(*this);
		}
		
		void serialize (std::string const & str) {
			string_size_type n = str.size();
			stream.write(reinterpret_cast<char const *>(&n), sizeof(string_size_type));
			if (!n) return;
			stream.write(&str[0], n);
		}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value, int>::type = 0> void serialize (std::vector<T> const & value) {
			size_type n = value.size();
			stream.write(reinterpret_cast<char const *>(&n), sizeof(size_type));
			if (!n) return;
			stream.write(reinterpret_cast<char const *>(value.data()), n * sizeof(T));
		}
		
		template <typename T, typename std::enable_if<!std::is_pod<T>::value, int>::type = 0> void serialize (std::vector<T> const & value) {
			size_type n = value.size();
			stream.write(reinterpret_cast<char const *>(&n), sizeof(size_type));
			if (!n) return;
			for (auto const & v : value) {
				serialize(v);
			}
		}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value, int>::type = 0> void serialize (std::unordered_set<T> const & value) {
			size_type n = value.size();
			stream.write(reinterpret_cast<char const *>(&n), sizeof(size_type));
			if (!n) return;
			stream.write(reinterpret_cast<char const *>(value.data()), n * sizeof(T));
		}
		
		template <typename T, typename std::enable_if<!std::is_pod<T>::value, int>::type = 0> void serialize (std::unordered_set<T> const & value) {
			size_type n = value.size();
			stream.write(reinterpret_cast<char const *>(&n), sizeof(size_type));
			if (!n) return;
			for (auto const & v : value) {
				serialize(v);
			}
		}
		
		template <typename K, typename V> void serialize (std::unordered_map<K, V> const & value) {
			size_type n = value.size();
			stream.write(reinterpret_cast<char const *>(&n), sizeof(size_type));
			if (!n) return;
			for (auto const & i : value) {
				serialize(i.first);
				serialize(i.second);
			}
		} 
		
	};
	
	struct deserializer {
		
		std::ifstream stream;
		
		deserializer() = delete;
		deserializer(std::string const & path) : stream(path, std::ios::binary) {}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value && !std::is_array<T>::value, int>::type = 0> void deserialize (T & t) {
			stream.read(reinterpret_cast<char *>(&t), sizeof(T));
		}
		
		template <typename T, typename std::enable_if<std::is_same<typename T::is_seasnake, std::true_type>::value, int>::type = 0> void deserialize (T & t) {
			t.deserialize(*this);
		}
		
		void deserialize(std::string & str) {
			string_size_type n;
			stream.read(reinterpret_cast<char *>(&n), sizeof(string_size_type));
			str.resize(n);
			stream.read(&str[0], n);
		}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value, int>::type = 0> void deserialize (std::vector<T> & t) {
			size_type n;
			stream.read(reinterpret_cast<char *>(&n), sizeof(size_type));
			t.resize(n);
			stream.read(t.data(), n * sizeof(T));
		}
		
		template <typename T, typename std::enable_if<!std::is_pod<T>::value, int>::type = 0> void deserialize (std::vector<T> & t) {
			size_type n;
			stream.read(reinterpret_cast<char *>(&n), sizeof(size_type));
			t.resize(n);
			for (auto & i : t) {
				deserialize(i);
			}
		}
		
		template <typename T> void deserialize (std::unordered_set<T> & t) {
			size_type n;
			stream.read(reinterpret_cast<char *>(&n), sizeof(size_type));
			for (size_type i = 0; i < n; i++) {
				T v;
				deserialize(v);
				t.insert(v);
			}
		}
		
		template <typename K, typename V> void deserialize (std::unordered_map<K, V> & t) {
			size_type n;
			stream.read(reinterpret_cast<char *>(&n), sizeof(size_type));
			for (size_type i = 0; i < n; i++) {
				K k;
				V v;
				deserialize(k);
				deserialize(v);
				t[k] = v;
			}
		}
		
	};

}
