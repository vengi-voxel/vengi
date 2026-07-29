/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace core {

class UUID {
private:
	uint64_t _data[2] = {0, 0};

public:
	UUID() = default;
	explicit UUID(const String &uuid);
	explicit UUID(uint64_t id0, uint64_t id1) : _data{id0, id1} {
	}
	explicit UUID(uint32_t id) : _data{id, 0} {
	}

	uint64_t data0() const {
		return _data[0];
	}
	uint64_t data1() const {
		return _data[1];
	}

	UUID &operator=(const String &uuid);

	bool operator==(const UUID &other) const {
		return _data[0] == other._data[0] && _data[1] == other._data[1];
	}
	bool operator!=(const UUID &other) const {
		return !(*this == other);
	}

	String str() const;
	bool isValid() const {
		return _data[0] != 0 || _data[1] != 0;
	}

	static UUID generate();
};

struct UUIDHash {
	size_t operator()(const core::UUID &p) const {
		uint64_t v1 = p.data0();
		uint64_t v2 = p.data1();
		// mix the two 64-bit parts into a size_t using a variant of boost::hash_combine
		uint64_t res = v1;
		res ^= v2 + 0x9e3779b97f4a7c15ULL + (res << 6) + (res >> 2);
		return (size_t)res;
	}
};

} // namespace core
