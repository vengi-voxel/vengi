/**
 * @file
 * @note Shamelessly ripped of of apitest https://github.com/nvMcJohn/apitest
 */

#pragma once

#include "Types.h"
#include "core/collection/Vector.h"

namespace video {

/**
 * @ingroup Video
 * @sa PersistentMappingBuffer
 */
class BufferLockMgr {
private:
	struct BufferRange {
		size_t start;
		size_t length;

		inline bool overlaps(const BufferRange &rhs) const {
			return start < (rhs.start + rhs.length) && rhs.start < (start + length);
		}
	};

	struct BufferLock {
		BufferRange range;
		IdPtr syncObj;
	};

	void wait(IdPtr syncObj);
	void cleanup(BufferLock &lock);

	using BufferLockArray = core::Vector<BufferLock, 2048>;
	BufferLockArray _locks;

	const bool _cpuUpdates;
public:
	/**
	 * @param[in] cpuUpdates Whether it's the CPU (true) that updates, or the GPU (false)
	 */
	BufferLockMgr(bool cpuUpdates = true);
	~BufferLockMgr();

	void waitForLockedRange(size_t lockBeginBytes, size_t lockLength);
	void lockRange(size_t lockBeginBytes, size_t lockLength);
};

}
