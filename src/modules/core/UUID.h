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
	UUID();
	UUID(const String &uuid);
	UUID(const UUID &other);
	UUID(UUID &&other) noexcept;
	~UUID();

	UUID &operator=(const String &uuid);
	UUID &operator=(const UUID &other);
	UUID &operator=(UUID &&other) noexcept;

	bool operator==(const UUID &other) const;
	bool operator!=(const UUID &other) const;

	String str() const;
	bool isValid() const;

	static UUID generate();
};

} // namespace core
