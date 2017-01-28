#include "time.hh"

blueshift::time::point::point(std::string const & etagi) {
	
	std::string etag = etagi;
	strops::trim(etag, '"');
	
	if (!etag.size()) return;
	
	std::string::const_iterator bi, ei;
	bi = ei = etag.begin();
	for (; ei != etag.end() && *ei != '_'; ei++);
	
	uint8_t * sec_d = reinterpret_cast<uint8_t *>(&data.tv_sec);
	for (size_t i = 0; i < sizeof(data.tv_sec); i++) {
		char vh = *bi;
		char vhs = vh > 57 ? vh - 55 : vh - 48;
		sec_d[i] = vhs << 4;
		if (++bi == ei) break;
		char vl = *bi;
		sec_d[i] |= vl > 57 ? vl - 55 : vl - 48;
		if (++bi == ei) break;
	}
	
	if (ei++ == etag.end() || ei == etag.end()) return;
	bi = ei;
	ei = etag.end();
	
	uint8_t * nsec_d = reinterpret_cast<uint8_t *>(&data.tv_nsec);
	for (size_t i = 0; i < sizeof(data.tv_nsec); i++) {
		char vh = *bi;
		char vhs = vh > 57 ? vh - 55 : vh - 48;
		nsec_d[i] = vhs << 4;
		if (++bi == ei) break;
		char vl = *bi;
		nsec_d[i] |= vl > 57 ? vl - 55 : vl - 48;
		if (++bi == ei) break;
	}
}

static constexpr uint8_t hmask = 0b11110000;
static constexpr uint8_t lmask = 0b00001111;

std::string blueshift::time::point::generate_etag() {
	std::string r {"\""};
	
	uint8_t * sec_d = reinterpret_cast<uint8_t *>(&data.tv_sec);
	size_t sec_dl = sizeof(data.tv_sec);
	
	for (ssize_t i = sec_dl - 1; i >= 0; i--) {
		if (!*(sec_d + i)) sec_dl--;
		else break;
	}
	
	for (size_t i = 0; i < sec_dl; i++) {
		uint8_t cur = *(sec_d + i);
		char vh = (cur & hmask) >> 4;
		char vl = cur & lmask;
		r += vh > 9 ? 55 + vh : 48 + vh;
		r += vl > 9 ? 55 + vl : 48 + vl;
	}
	
	if (data.tv_nsec) {
	
		r += '_';
		
		uint8_t * nsec_d = reinterpret_cast<uint8_t *>(&data.tv_nsec);
		size_t nsec_dl = sizeof(data.tv_nsec);
		
		for (ssize_t i = nsec_dl - 1; i >= 0; i--) {
			if (!*(nsec_d + i)) nsec_dl--;
			else break;
		}
		
		for (size_t i = 0; i < nsec_dl; i++) {
			uint8_t cur = *(nsec_d + i);
			char vh = (cur & hmask) >> 4;
			char vl = cur & lmask;
			r += vh > 9 ? 55 + vh : 48 + vh;
			r += vl > 9 ? 55 + vl : 48 + vl;
		}
	
	}
	
	r += "\"";
	return r;
}

