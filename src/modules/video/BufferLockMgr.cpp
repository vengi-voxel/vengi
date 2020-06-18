/**
 * @file
 */

#include "BufferLockMgr.h"
#include "Renderer.h"

namespace video {

BufferLockMgr::BufferLockMgr(bool cpuUpdates) :
		_cpuUpdates(cpuUpdates) {
}

BufferLockMgr::~BufferLockMgr() {
	for (auto it = _locks.begin(); it != _locks.end(); ++it) {
		cleanup(*it);
	}
}

void BufferLockMgr::waitForLockedRange(size_t lockBeginBytes, size_t lockLength) {
	const BufferRange testRange {lockBeginBytes, lockLength};
	BufferLockArray swapLocks;
	for (auto it = _locks.begin(); it != _locks.end(); ++it) {
		if (testRange.overlaps(it->range)) {
			wait(it->syncObj);
			cleanup(*it);
		} else {
			swapLocks.push_back(*it);
		}
	}

	_locks = swapLocks;
}

void BufferLockMgr::lockRange(size_t lockBeginBytes, size_t lockLength) {
	const BufferRange newRange{lockBeginBytes, lockLength};
	const IdPtr syncName = video::genSync();
	const BufferLock newLock{newRange, syncName};
	_locks.push_back(newLock);
}

void BufferLockMgr::wait(IdPtr syncObj) {
	if (_cpuUpdates) {
		static constexpr uint64_t kOneSecondInNanoSeconds = 1000000000;
		uint64_t waitDuration = 0;
		bool syncFlushCommands = false;
		for (;;) {
			const bool waitRet = waitForClientSync(syncObj, waitDuration, syncFlushCommands);
			if (waitRet) {
				return;
			}

			// After the first time, need to start flushing, and wait for a looong time.
			syncFlushCommands = true;
			waitDuration = kOneSecondInNanoSeconds;
		}
	} else {
		waitForSync(syncObj);
	}
}

void BufferLockMgr::cleanup(BufferLock &bufferLock) {
	deleteSync(bufferLock.syncObj);
}

}
