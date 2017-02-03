#pragma once

#include "com.hh"

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <type_traits>
#include <fstream>

namespace blueshift::seasnake {
	
	typedef uint8_t byte;
	typedef uint32_t default_size_t;
	
	static constexpr char const * ssdb_header = "SSDB";
	
	struct serializer {
		std::ofstream out;
		serializer() = delete;
		serializer(std::string const & path);
		~serializer();
		void write(byte const * p, size_t len);
	};
	
	struct deserializer {
		std::ifstream in;
		deserializer() = delete;
		deserializer(std::string const & path);
		~deserializer();
		void read(byte * p, size_t len);
	};
	
	template <typename T, typename S = default_size_t> struct ssdb {
		ssdb() = default;
		T root {};
		void save (std::string const & path) {
			serializer ss {path};
			ss.write(reinterpret_cast<byte const *>(ssdb_header), 4);
			root.serialize(ss);
		}
		void load (std::string const & path) {
			deserializer ds {path};
			byte hs [4];
			ds.read(hs, 4);
			if (memcmp(hs, ssdb_header, 4)) srcthrow("header mismatch!");
			root.deserialize(ds);
		}
	};
	
	template <typename T> struct sstype_scalar {
		static_assert(std::is_scalar<T>(), "sstype_scalar requires a scalar type");
		typedef T value_type;
		T value;
		sstype_scalar() : value(0) {}
		sstype_scalar(T const & v) : value(v) {}
		operator T & () { return value; }
		operator T const & () const { return value; }
		inline sstype_scalar<T> & operator = (T const & other) { value = other; return *this; }
		inline sstype_scalar<T> & operator = (sstype_scalar<T> const & other) { value = other.value; return *this; }
		inline bool operator == (T const & other) const { return other == value; }
		inline bool operator == (sstype_scalar<T> const & other) const { return other.value == value; }
		void serialize (serializer & s) const {
			s.write(reinterpret_cast<byte const *>(&this->value), sizeof(T));
		}
		void deserialize (deserializer & ds) {
			ds.read(reinterpret_cast<byte *>(&this->value), sizeof(T));
		}
	};
	
	template <typename T> struct sstype_pod {
		static_assert(std::is_pod<T>(), "sstype_pod requires a scalar type or POD struct");
		typedef T value_type;
		T value;
		sstype_pod() : value {} {}
		sstype_pod(T const & v) : value(v) {}
		operator T & () { return value; }
		operator T const & () const { return value; }
		inline sstype_pod<T> & operator = (T const & other) { value = other; return *this; }
		inline sstype_pod<T> & operator = (sstype_pod<T> const & other) { value = other.value; return *this; }
		inline bool operator == (T const & other) const { return other == value; }
		inline bool operator == (sstype_pod<T> const & other) const { return other.value == value; }
		void serialize (serializer & s) const {
			s.write(reinterpret_cast<byte const *>(&this->value), sizeof(T));
		}
		void deserialize (deserializer & ds) {
			ds.read(reinterpret_cast<byte *>(&this->value), sizeof(T));
		}
	};
	
	template <typename T, typename S = default_size_t> struct ss_string_t {
		static_assert(std::is_integral<S>(), "ss_string_t requires an integral type as the template second parameter (size)");
		typedef T value_type;
		T value;
		ss_string_t() : value{} {}
		ss_string_t(T const & v) : value(v) {}
		operator T & () { return value; }
		operator T const & () const { return value; }
		inline ss_string_t<T> & operator = (T const & other) { value = other; return *this; }
		inline ss_string_t<T> & operator = (ss_string_t<T> const & other) { value = other.value; return *this; }
		inline bool operator == (T const & other) const { return other == value; }
		inline bool operator == (ss_string_t<T> const & other) const { return other.value == value; }
		void serialize (serializer & s) const {
			S sz = value.size();
			s.write(reinterpret_cast<byte const *>(&sz), sizeof(sz));
			s.write(reinterpret_cast<byte const *>(value.data()), sz);
		}
		void deserialize (deserializer & ds) {
			S sz;
			ds.read(reinterpret_cast<byte *>(&sz), sizeof(S));
			value.resize(sz);
			ds.read(reinterpret_cast<byte *>(&value[0]), sz);
		}
	};
	
	template <typename T, typename S = default_size_t> struct sstype_array {
		static_assert(std::is_integral<S>(), "sstype_array requires an integral type as the template second parameter (size)");
		typedef T value_type;
		T value;
		sstype_array() : value{} {}
		sstype_array(T const & v) : value(v) {}
		operator T & () { return value; }
		operator T const & () const { return value; }
		inline sstype_array<T> & operator = (T const & other) { value = other; return *this; }
		inline sstype_array<T> & operator = (sstype_array<T> const & other) { value = other.value; return *this; }
		inline bool operator == (T const & other) const { return other == value; }
		inline bool operator == (sstype_array<T> const & other) const { return other.value == value; }
		void serialize (serializer & s) const {
			S sz = value.size();
			s.write(reinterpret_cast<byte const *>(&sz), sizeof(sz));
			for (auto const & i : value) {
				i.serialize(s);
			}
		}
		void deserialize (deserializer & ds) {
			value.clear();
			S sz;
			ds.read(reinterpret_cast<byte *>(&sz), sizeof(S));
			value.reserve(sz);
			for (S i = 0; i < sz; i++) {
				typename value_type::value_type v {};
				v.deserialize(ds);
				value.push_back(v);
			}
		}
	};
	
	template <typename T, typename S = default_size_t> struct sstype_set {
		static_assert(std::is_integral<S>(), "sstype_set requires an integral type as the template second parameter (size)");
		typedef T value_type;
		T value;
		sstype_set() : value{} {}
		sstype_set(T const & v) : value(v) {}
		operator T & () { return value; }
		operator T const & () const { return value; }
		inline sstype_set<T> & operator = (T const & other) { value = other; return *this; }
		inline sstype_set<T> & operator = (sstype_set<T> const & other) { value = other.value; return *this; }
		inline bool operator == (T const & other) const { return other == value; }
		inline bool operator == (sstype_set<T> const & other) const { return other.value == value; }
		void serialize (serializer & s) const {
			S sz = value.size();
			s.write(reinterpret_cast<byte const *>(&sz), sizeof(sz));
			for (auto const & i : value) {
				i.serialize(s);
			}
		}
		void deserialize (deserializer & ds) {
			value.clear();
			S sz;
			ds.read(reinterpret_cast<byte *>(&sz), sizeof(S));
			value.reserve(sz);
			for (S i = 0; i < sz; i++) {
				typename value_type::value_type v {};
				v.deserialize(ds);
				value.insert(v);
			}
		}
	};
	
	template <typename T, typename S = default_size_t> struct sstype_map {
		static_assert(std::is_integral<S>(), "sstype_map requires an integral type as the template second parameter (size)");
		typedef T value_type;
		T value;
		sstype_map() : value{} {}
		sstype_map(T const & v) : value(v) {}
		operator T & () { return value; }
		operator T const & () const { return value; }
		inline sstype_map<T> & operator = (T const & other) { value = other; return *this; }
		inline sstype_map<T> & operator = (sstype_map<T> const & other) { value = other.value; return *this; }
		inline bool operator == (T const & other) const { return other == value; }
		inline bool operator == (sstype_map<T> const & other) const { return other.value == value; }
		typename T::mapped_type & operator [] (typename T::key_type i) {
			return value[i];
		}
		void serialize (serializer & s) const {
			S sz = value.size();
			s.write(reinterpret_cast<byte const *>(&sz), sizeof(sz));
			for (auto const & i : value) {
				i.first.serialize(s);
				i.second.serialize(s);
			}
		}
		void deserialize (deserializer & ds) {
			value.clear();
			S sz;
			ds.read(reinterpret_cast<byte *>(&sz), sizeof(S));
			value.reserve(sz);
			for (S i = 0; i < sz; i++) {
				typename value_type::key_type k {};
				typename value_type::mapped_type v {};
				k.deserialize(ds);
				v.deserialize(ds);
				value[k] = v;
			}
		}
	};
	
	template <typename SS> struct ss_hash {
		uint64_t operator () (SS const & s) const {
			return std::hash<typename SS::value_type>{}(s.value);
		}
	};
	
	template <typename T, typename S = default_size_t> using ss_vector_t = sstype_array<std::vector<T>, S>;
	template <typename T, typename S = default_size_t, typename H = ss_hash<T>> using ss_uset_t = sstype_set<std::unordered_set<T, H>, S>;
	
	template <typename T1, typename T2, typename S = default_size_t, typename H = ss_hash<T1>> using ss_umap_t = sstype_map<std::unordered_map<T1, T2, H>, S>;
	
	typedef sstype_scalar<int8_t> ss_int8;
	typedef sstype_scalar<uint8_t> ss_uint8;
	typedef sstype_scalar<int16_t> ss_int16;
	typedef sstype_scalar<uint16_t> ss_uint16;
	typedef sstype_scalar<int32_t> ss_int32;
	typedef sstype_scalar<uint32_t> ss_uint32;
	typedef sstype_scalar<int64_t> ss_int64;
	typedef sstype_scalar<uint64_t> ss_uint64;
	typedef sstype_scalar<float> ss_float;
	typedef sstype_scalar<double> ss_double;
	
	typedef ss_string_t<std::string> ss_string;
	
};
