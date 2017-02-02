#include "seasnake.hh"

#include <fstream>

blueshift::seasnake::serializer::serializer(const std::string& path) : out { path, std::ios::binary | std::ios::out } { }

blueshift::seasnake::serializer::~serializer() { }

void blueshift::seasnake::serializer::write(byte const * p, size_t len) {
	out.write(reinterpret_cast<char const *>(p), len);
}

blueshift::seasnake::deserializer::deserializer(const std::string& path) : in { path, std::ios::binary | std::ios::out } { }

blueshift::seasnake::deserializer::~deserializer() { }

void blueshift::seasnake::deserializer::read(byte * p, size_t len) {
	in.read(reinterpret_cast<char *>(p), len);
}
