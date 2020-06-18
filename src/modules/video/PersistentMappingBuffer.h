/**
 * @file
 */

#pragma once

#include "Types.h"
#include "BufferLockMgr.h"

namespace video {

/**
 * @brief Directly write to the gpu memory. The pointer is valid until shutdown is called.
 * @note Don't forget to sync!
 *
 * @sa BufferLockMgr
 * @ingroup Video
 */
class PersistentMappingBuffer {
private:
	const size_t _size;
	uint8_t*_memory = nullptr;
	video::Id _handle = video::InvalidId;
	BufferLockMgr _lockMgr;
public:
	PersistentMappingBuffer(size_t size);

	bool init();
	void shutdown();

	/**
	 * @brief Writes data into the buffer at the given position
	 * @note Adds a sync point that can get queried via @c wait()
	 */
	bool write(const uint8_t *data, size_t offset, size_t size);
	/**
	 * @brief Checks whether a previous @c write() call is done already.
	 * @return If this failes, @c false is returned
	 */
	bool wait(size_t offset, size_t size);

	size_t size() const;
	video::Id handle();
	/**
	 * @note Don't forget to lock manually if you use this!
	 */
	uint8_t* memory();
};

inline uint8_t* PersistentMappingBuffer::memory() {
	return _memory;
}

inline video::Id PersistentMappingBuffer::handle() {
	return _handle;
}

inline size_t PersistentMappingBuffer::size() const {
	return _size;
}

}
