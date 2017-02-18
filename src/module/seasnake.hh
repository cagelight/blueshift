#pragma once

#include <type_traits>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace seasnake {
	
// ================================================================
	
	typedef uint8_t byte;
	
	inline std::vector<byte> encode_autolength (size_t size) {
		
		std::vector<byte> bytes {};
		
		size_t sizem = size << 3;
		
		if (size < 32) {
			byte init = static_cast<byte>(sizem);
			bytes.push_back(init);
			return bytes;
		}
		
		bytes.push_back(0);
		
		if (size >= 32) bytes.push_back(reinterpret_cast<byte *>(&sizem)[1]); else goto end;
		if (size >= 8192) bytes.push_back(reinterpret_cast<byte *>(&sizem)[2]); else goto end;
		if (size >= 2097152) bytes.push_back(reinterpret_cast<byte *>(&sizem)[3]); else goto end;
		if (size >= 536870912) bytes.push_back(reinterpret_cast<byte *>(&sizem)[4]); else goto end;
		if (size >= 137438953472) bytes.push_back(reinterpret_cast<byte *>(&sizem)[5]); else goto end;
		if (size >= 35184372088832) bytes.push_back(reinterpret_cast<byte *>(&sizem)[6]); else goto end;
		if (size >= 9007199254740992) bytes.push_back(reinterpret_cast<byte *>(&sizem)[7]); else goto end;
		
		end:
		bytes[0] = ((bytes.size() - 1)) | reinterpret_cast<byte *>(&sizem)[0];
		
		return bytes;
	}
	
	inline size_t decode_autolength_count(byte head) {
		return (head & 0b00000111) + 1;
	}
	
	inline size_t decode_autolength (byte const * data) {
		size_t r = 0;
		for (size_t i = 0; i < 8; i++) {
			if (i < (size_t)(*data & 0b00000111) + 1) {
				r |= static_cast<size_t>(data[i]) << (i * 8);
			} else {
				reinterpret_cast<byte *>(&r)[i] = 0;
			}
		}
		r >>= 3;
		return r;
	}

	struct serializer {
		
		std::ofstream stream;
		
		serializer() = delete;
		serializer(std::string const & path) : stream(path, std::ios::binary) {}
		
		void serialize_autosize(size_t s) {
			auto v = encode_autolength(s);
			stream.write(reinterpret_cast<char const *>(v.data()), v.size());
		}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value && !std::is_array<T>::value, int>::type = 0> void serialize (T const & value) {
			stream.write(reinterpret_cast<char const *>(&value), sizeof(T));
		}
		
		template <typename T, typename std::enable_if<std::is_same<typename T::is_seasnake, std::true_type>::value, int>::type = 0> void serialize (T const & value) {
			value.serialize(*this);
		}
		
		void serialize (std::string const & str) {
			size_t n = str.size();
			serialize_autosize(n);
			if (!n) return;
			stream.write(&str[0], n);
		}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value, int>::type = 0> void serialize (std::vector<T> const & value) {
			size_t n = value.size();
			serialize_autosize(n);
			if (!n) return;
			stream.write(reinterpret_cast<char const *>(value.data()), n * sizeof(T));
		}
		
		template <typename T, typename std::enable_if<!std::is_pod<T>::value, int>::type = 0> void serialize (std::vector<T> const & value) {
			size_t n = value.size();
			serialize_autosize(n);
			if (!n) return;
			for (auto const & v : value) {
				serialize(v);
			}
		}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value, int>::type = 0> void serialize (std::unordered_set<T> const & value) {
			size_t n = value.size();
			serialize_autosize(n);
			if (!n) return;
			stream.write(reinterpret_cast<char const *>(value.data()), n * sizeof(T));
		}
		
		template <typename T, typename std::enable_if<!std::is_pod<T>::value, int>::type = 0> void serialize (std::unordered_set<T> const & value) {
			size_t n = value.size();
			serialize_autosize(n);
			if (!n) return;
			for (auto const & v : value) {
				serialize(v);
			}
		}
		
		template <typename K, typename V> void serialize (std::unordered_map<K, V> const & value) {
			size_t n = value.size();
			serialize_autosize(n);
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
		
		size_t deserialize_autosize() {
			size_t in = 0;
			size_t byte_count = decode_autolength_count(stream.peek());
			stream.read(reinterpret_cast<char *>(&in), byte_count);
			return decode_autolength(reinterpret_cast<byte *>(&in));
		}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value && !std::is_array<T>::value, int>::type = 0> void deserialize (T & t) {
			stream.read(reinterpret_cast<char *>(&t), sizeof(T));
		}
		
		template <typename T, typename std::enable_if<std::is_same<typename T::is_seasnake, std::true_type>::value, int>::type = 0> void deserialize (T & t) {
			t.deserialize(*this);
		}
		
		void deserialize(std::string & str) {
			size_t n = deserialize_autosize();
			str.resize(n);
			stream.read(&str[0], n);
		}
		
		template <typename T, typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value, int>::type = 0> void deserialize (std::vector<T> & t) {
			size_t n = deserialize_autosize();
			t.resize(n);
			stream.read(t.data(), n * sizeof(T));
		}
		
		template <typename T, typename std::enable_if<!std::is_pod<T>::value, int>::type = 0> void deserialize (std::vector<T> & t) {
			size_t n = deserialize_autosize();
			t.resize(n);
			for (auto & i : t) {
				deserialize(i);
			}
		}
		
		template <typename T> void deserialize (std::unordered_set<T> & t) {
			size_t n = deserialize_autosize();
			for (size_t i = 0; i < n; i++) {
				T v;
				deserialize(v);
				t.insert(v);
			}
		}
		
		template <typename K, typename V> void deserialize (std::unordered_map<K, V> & t) {
			size_t n = deserialize_autosize();
			for (size_t i = 0; i < n; i++) {
				K k;
				V v;
				deserialize(k);
				deserialize(v);
				t[k] = v;
			}
		}
		
	};

}
