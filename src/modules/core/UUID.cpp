/**
 * @file
 */

#include "UUID.h"
#include "core/Hash.h"
#include <cstdio>
#include <random>

namespace core {

UUID::UUID() {
	_data[0] = 0;
	_data[1] = 0;
}

UUID::UUID(const String &uuid) {
	*this = uuid;
}

UUID::UUID(const UUID &other) {
	_data[0] = other._data[0];
	_data[1] = other._data[1];
}

UUID::UUID(UUID &&other) noexcept {
	_data[0] = other._data[0];
	_data[1] = other._data[1];
}

UUID::~UUID() {
}

UUID &UUID::operator=(const String &uuid) {
	// parse a UUID string of form xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
	_data[0] = 0;
	_data[1] = 0;
	if (uuid.size() != 36) {
		return *this;
	}
	auto hexToByte = [](char c) -> int {
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'a' && c <= 'f')
			return 10 + (c - 'a');
		if (c >= 'A' && c <= 'F')
			return 10 + (c - 'A');
		return -1;
	};
	uint8_t bytes[16] = {0};
	int bi = 0;
	for (size_t i = 0; i < uuid.size(); ++i) {
		char c = uuid[i];
		if (c == '-')
			continue;
		int hi = hexToByte(c);
		++i;
		if (i >= uuid.size())
			return *this;
		int lo = hexToByte(uuid[i]);
		if (hi < 0 || lo < 0)
			return *this;
		bytes[bi++] = (uint8_t)((hi << 4) | lo);
		if (bi > 16)
			return *this;
	}
	if (bi != 16)
		return *this;
	// fill data in platform independent way: treat as two uint64_t in big endian
	uint64_t high = 0;
	uint64_t low = 0;
	for (int i = 0; i < 8; ++i) {
		high = (high << 8) | bytes[i];
	}
	for (int i = 8; i < 16; ++i) {
		low = (low << 8) | bytes[i];
	}
	_data[0] = high;
	_data[1] = low;
	return *this;
}

UUID &UUID::operator=(const UUID &other) {
	_data[0] = other._data[0];
	_data[1] = other._data[1];
	return *this;
}

UUID &UUID::operator=(UUID &&other) noexcept {
	_data[0] = other._data[0];
	_data[1] = other._data[1];
	return *this;
}

bool UUID::operator==(const UUID &other) const {
	return _data[0] == other._data[0] && _data[1] == other._data[1];
}

bool UUID::operator!=(const UUID &other) const {
	return !(*this == other);
}

String UUID::str() const {
	if (!isValid()) {
		return "";
	}
	// convert _data to bytes then to hex with dashes
	uint8_t bytes[16];
	uint64_t high = _data[0];
	uint64_t low = _data[1];
	for (int i = 7; i >= 0; --i) {
		bytes[i] = (uint8_t)(high & 0xFF);
		high >>= 8;
	}
	for (int i = 15; i >= 8; --i) {
		bytes[i] = (uint8_t)(low & 0xFF);
		low >>= 8;
	}
	static const char *hex = "0123456789ABCDEF";
	char buf[37];
	int p = 0;
	for (int i = 0; i < 16; ++i) {
		if (i == 4 || i == 6 || i == 8 || i == 10) {
			buf[p++] = '-';
		}
		buf[p++] = hex[(bytes[i] >> 4) & 0xF];
		buf[p++] = hex[bytes[i] & 0xF];
	}
	buf[p] = '\0';
	return buf;
}

bool UUID::isValid() const {
	return _data[0] != 0 || _data[1] != 0;
}

UUID UUID::generate() {
	return UUID(core::generateUUID());
}

} // namespace core
