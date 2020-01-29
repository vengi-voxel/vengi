/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include "core/ReadWriteLock.h"
#include "core/collection/Array.h"
#include "core/String.h"
#include <stdint.h>

#ifdef _MSC_VER
#define __attribute__(x)
#endif

namespace core {

/**
 * @brief The memguard class implements memory consistency checks
 * like underflow or overflow of the allocated memory.
 */
class MemGuard {
private:
	static constexpr uint32_t _headGuard = 0xDEADBEEF;
	static constexpr uint32_t _tailGuard = 0xBADDCAFE;
	static constexpr uint32_t _footerGuard = 0x8BADF00D;

	struct Chunk {
		Chunk* _nextChunk;
		uint32_t _headGuard;
		size_t _size;
		uint32_t _tailGuard;
	};
	static constexpr size_t _overheadSize = sizeof(Chunk) + sizeof(uint32_t);

	std::string _name;
	uint32_t _chunkAmount = 0u;
	uint32_t _byteAmount = 0u;
	core::ReadWriteLock _rwLock;

	core::Array<Chunk*, 13> _chunkBuckets = {{ nullptr, nullptr, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
			nullptr, nullptr }};

	Chunk* chunk_cast(void* const ptr) const;

	void* pointer_cast(Chunk* const chunk) const;

	uint32_t* footer_cast(Chunk* const chunk) const;

	void checkIntegrity(Chunk* const chunk) const;

public:
	MemGuard(const std::string& name);

	~MemGuard();

	void free(void* ptr);

	void* alloc(size_t size, bool zeroFill = false) __attribute__ ((malloc));

	void* realloc(void* ptr, size_t size);
};

inline MemGuard::Chunk* MemGuard::chunk_cast(void* const ptr) const {
	Chunk* chunk = static_cast<Chunk*>(ptr) - 1;
	checkIntegrity(chunk);
	return chunk;
}

inline void* MemGuard::pointer_cast(MemGuard::Chunk* const chunk) const {
	return chunk + 1;
}

inline uint32_t* MemGuard::footer_cast(MemGuard::Chunk* const chunk) const {
	uint8_t* bytePtr = reinterpret_cast<uint8_t*>(pointer_cast(chunk));
	return reinterpret_cast<uint32_t*>(bytePtr + chunk->_size);
}

inline void MemGuard::checkIntegrity(MemGuard::Chunk* const chunk) const {
	core_assert_always(chunk->_headGuard == _headGuard);
	core_assert_always(chunk->_tailGuard == _tailGuard);
	core_assert_always(*footer_cast(chunk) == _footerGuard);
}


}
