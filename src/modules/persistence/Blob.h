/**
 * @file
 */

#pragma once

#include <string.h>
#include <stdint.h>

namespace persistence {

struct Blob {
	uint8_t *data = nullptr;
	size_t length = 0u;

	constexpr Blob() {
	}

	Blob(uint8_t* data, size_t length);

	void release();
};

}
