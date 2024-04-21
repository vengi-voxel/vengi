/**
 * @file
 */

#include "BufferUtil.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include "math/Functions.h"

namespace util {

static uint32_t maxIndexValue(const uint8_t *in, size_t inSize, size_t inIndexSize) {
	uint32_t v = 0u;
	const uint8_t *buf = in;
	for (size_t i = 0; i < inSize / inIndexSize; ++i) {
		if (inIndexSize == 4) {
			const uint32_t index = *(const uint32_t*)buf;
			v = core_max(index, v);
		} else if (inIndexSize == 2) {
			const uint16_t index = *(const uint16_t*)buf;
			v = core_max(index, v);
		} else if (inIndexSize == 1) {
			v = core_max(*buf, v);
		}
		buf += inIndexSize;
	}
	return v;
}

template<class SOURCE, class TARGET>
bool compress(const uint8_t *in, size_t inSize, uint8_t *buf, size_t bufSize) {
	const size_t indices = inSize / sizeof(SOURCE);
	const size_t outIndices = bufSize / sizeof(TARGET);
	if (outIndices < indices) {
		return false;
	}
	for (size_t i = 0; i < indices; ++i) {
		const SOURCE index = *(const SOURCE*)in;
		*(TARGET*)buf = (TARGET)index;
		in += sizeof(SOURCE);
		buf += sizeof(TARGET);
	}
	return true;
}

bool indexCompress(const uint8_t *in, size_t inSize, size_t inIndexSize, size_t &bytesPerIndex, uint8_t *buf, size_t bufSize) {
	// nothing can be done here
	if (inIndexSize == 1) {
		core_assert(inSize / inIndexSize <= bufSize);
		core_memcpy(buf, in, core_min(bufSize, inSize));
		bytesPerIndex = 1;
		return true;
	}

	uint32_t m = maxIndexValue(in, inSize, inIndexSize);
	const uint8_t l = math::logBase(256, m);
	if (l == 0u) {
		bytesPerIndex = 1u;
	} else if (l == 1u) {
		bytesPerIndex = 2u;
	} else {
		bytesPerIndex = 4u;
	}

	core_assert(inIndexSize >= bytesPerIndex);
	core_assert(inSize / inIndexSize * bytesPerIndex <= bufSize);

	// there is nothing to compress here - just copy the buffer
	if (inIndexSize == bytesPerIndex) {
		core_memcpy(buf, in, core_min(bufSize, inSize));
		return true;
	}

	if (inIndexSize == 2) {
		core_assert(bytesPerIndex == 1);
		return compress<uint16_t, uint8_t>(in, inSize, buf, bufSize);
	}
	if (inIndexSize == 4) {
		if (bytesPerIndex == 1) {
			return compress<uint32_t, uint8_t>(in, inSize, buf, bufSize);
		} else {
			core_assert(bytesPerIndex == 2);
			return compress<uint32_t, uint16_t>(in, inSize, buf, bufSize);
		}
	}
	return false;
}

}
