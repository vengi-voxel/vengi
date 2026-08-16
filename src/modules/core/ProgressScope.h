/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/IProgress.h"
#include "core/concurrent/Atomic.h"

namespace core {

/**
 * @brief Returns the ambient progress sink, or @c NullProgress if none is active.
 *
 * Prefer this when reporting from leaf helpers that should not take an @c IProgress
 * parameter. Pair with @c ProgressScope at the call site that owns the sink.
 */
IProgress &currentProgress();

/**
 * @brief Active ambient progress pointer, or @c nullptr when unset.
 *
 * Use this to skip work when no progress sink is installed (avoids locking
 * @c SharedProgress from hot loops).
 */
IProgress *currentProgressPtr();

/**
 * @brief Installs @p progress as the ambient sink and returns the previous one.
 */
IProgress *setCurrentProgress(IProgress *progress);

/**
 * @brief RAII ambient progress installation (nestable, visible to thread-pool workers).
 *
 * Scene jobs and loaders push a @c SharedProgress (or @c ProgressRange) for the
 * duration of background work. @c voxelutil algorithms then report via
 * @c currentProgress() / @c ParallelProgress without signature changes.
 *
 * Uses an atomic pointer (not thread-local) so @c app::for_parallel workers see the
 * same sink as the installing thread.
 */
class ProgressScope {
private:
	IProgress *_previous;

public:
	explicit ProgressScope(IProgress &progress);
	~ProgressScope();

	ProgressScope(const ProgressScope &) = delete;
	ProgressScope &operator=(const ProgressScope &) = delete;
};

/**
 * @brief Accumulates completed units across parallel workers into ambient progress.
 *
 * No-op when no @c ProgressScope is active. Safe to call from @c for_parallel chunks.
 */
class ParallelProgress {
private:
	IProgress *_progress;
	int _total;
	AtomicInt _done;

public:
	explicit ParallelProgress(int total)
		: _progress(currentProgressPtr()), _total(core_max(1, total)), _done(0) {
	}

	void add(int n) {
		if (_progress == nullptr || n <= 0) {
			return;
		}
		// SDL_AddAtomicInt (via AtomicInt::increment) returns the previous value.
		const int finished = _done.increment(n) + n;
		_progress->setProgress((float)finished / (float)_total);
	}
};

} // namespace core
