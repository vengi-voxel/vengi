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
	static constexpr char LUT[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
								   'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
								   'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
								   'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

	static CORE_FORCE_INLINE bool validbyte(const uint8_t c) {
		return c == '+' || c == '/' || isalnum(c);
	}
};

} // namespace io
