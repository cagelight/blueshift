#include "crypto.hh"

#include <botan/botan.h>
#include <botan/lookup.h>
#include <botan/cipher_mode.h>
using namespace Botan;

static AutoSeeded_RNG rng {};
static std::unique_ptr<HashFunction> skein_func = nullptr;

blueshift::byte_buffer blueshift::crypto::random(size_t len) {
	auto vec = rng.random_vec(len);
	byte_buffer bb {};
	bb.resize(len);
	memcpy(bb.data(), vec.data(), len);
	return bb;
}

blueshift::byte_buffer blueshift::crypto::hash_skein (std::string str) {
	if (!skein_func) {
		skein_func = HashFunction::create("Skein-512");
		if (!skein_func) srcthrow("Skein-512 could not be found!");
	}
	auto buf = skein_func->process(str);
	byte_buffer bb {};
	bb.resize(buf.size());
	memcpy(bb.data(), buf.data(), bb.size());
	return bb;
}

blueshift::byte_buffer blueshift::crypto::hash_skein (byte_buffer str) {
	if (!skein_func) {
		skein_func = HashFunction::create("Skein-512");
		if (!skein_func) srcthrow("Skein-512 could not be found!");
	}
	auto buf = skein_func->process(str.get_buffer());
	byte_buffer bb {};
	bb.resize(buf.size());
	memcpy(bb.data(), buf.data(), bb.size());
	return bb;
}
