/**
 * @file
 */

#include "ByteStream.h"
#include <SDL_stdinc.h>
#include <stdarg.h>

namespace io {

ByteStream::ByteStream(int size) {
	_buffer.reserve(size);
}

}
