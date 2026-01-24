/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include <ctype.h>
#include <stdint.h>

namespace io {

/**
 * @ingroup IO
 */
class Base64Stream {
protected:
	/**
	 * This array is a LUT specified in RFC 4648
	 */
	static const char LUT[];

	static CORE_FORCE_INLINE bool validbyte(const uint8_t c) {
		return c == '+' || c == '/' || isalnum(c);
	}
};

} // namespace io
