/**
 * @file
 */

#include "BufferedReadWriteStream.h"
#include <SDL_stdinc.h>
#include <stdarg.h>

namespace io {

BufferedReadWriteStream::BufferedReadWriteStream(int size) {
	_buffer.reserve(size);
}

}
