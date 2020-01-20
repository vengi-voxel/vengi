/**
 * @file
 */

#include "State.h"
#include "Blob.h"

namespace persistence {

Blob::Blob(uint8_t *_data, size_t _length) :
		data(_data), length(_length) {
}

void Blob::release() {
	State::freeBlob(data);
	data = nullptr;
	length = 0u;
}

}
