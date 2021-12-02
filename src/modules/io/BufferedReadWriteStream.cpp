/**
 * @file
 */

#include "BufferedReadWriteStream.h"

namespace io {

BufferedReadWriteStream::BufferedReadWriteStream(int size) {
	_buffer.reserve(size);
}

}
