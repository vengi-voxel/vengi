#include "MemGuard.h"
#include "Common.h"
#include <SDL.h>

namespace core {

MemGuard::MemGuard(const core::String& name) :
		_name(name), _rwLock(name) {
}

MemGuard::~MemGuard() {
	for (Chunk* chunk : _chunkBuckets) {
		free(pointer_cast(chunk));
	}

	core_assert(_chunkAmount == 0);
	core_assert(_byteAmount == 0);
}

void MemGuard::free(void* ptr) {
	if (ptr == nullptr) {
		return;
	}

	Chunk* const chunk = chunk_cast(ptr);

	{
		core::ScopedWriteLock scoped(_rwLock);

		_chunkAmount--;
		_byteAmount -= chunk->_size + _overheadSize;

		Chunk** prevBucket = &_chunkBuckets[(uintptr_t) chunk % _chunkBuckets.size()];
		for (;;) {
			Chunk* c = *prevBucket;
			if (c == nullptr) {
				break;
			}

			if (c == chunk) {
				*prevBucket = c->_nextChunk;
				break;
			}
			prevBucket = &c->_nextChunk;
		}
	}

	core_free(chunk);
}

void* MemGuard::alloc(size_t size, bool zeroFill) {
	size += _overheadSize;

	Chunk* mem = static_cast<Chunk*>(core_malloc(size));
	if (zeroFill) {
		core_memset(mem, 0, size);
	}

	mem->_headGuard = _headGuard;
	mem->_size = size - _overheadSize;
	mem->_tailGuard = _tailGuard;

	*footer_cast(mem) = _footerGuard;

	core::ScopedWriteLock scoped(_rwLock);

	_chunkAmount++;
	_byteAmount += size;

	mem->_nextChunk = _chunkBuckets[(uintptr_t) mem % _chunkBuckets.size()];
	_chunkBuckets[(uintptr_t) mem % _chunkBuckets.size()] = mem;

	return pointer_cast(mem);
}

void* MemGuard::realloc(void* ptr, size_t size) {
	Chunk* const chunk = chunk_cast(ptr);
	if (chunk->_size == size) {
		return ptr;
	}

	void* newPtr = alloc(size, false);
	core_memcpy(newPtr, ptr, core_min(chunk->_size, size));
	if (chunk->_size < size) {
		const size_t delta = size - chunk->_size;
		core_memset((uint8_t*) newPtr + chunk->_size, 0, delta);
	}

	free(ptr);

	checkIntegrity(chunk_cast(newPtr));

	return newPtr;
}

}
