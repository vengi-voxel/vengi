/**
 * @file
 */

#include "Progress.h"

#include "core/Log.h"
#include "core/TimeProvider.h"

#include <cstdio>

namespace voxedit {
namespace printing {

ProgressTimer::ProgressTimer(const char *tag, int total, bool verbose)
	: _tag(tag), _total(total), _startMs(core::TimeProvider::systemMillis()), _verbose(verbose) {
	if (!_verbose) {
		return;
	}
	fprintf(stderr, "3dprint %s: starting (%d items)\n", _tag, _total);
	fflush(stderr);
	Log::info("3dprint %s: starting (%d items)", _tag, _total);
}

ProgressTimer::~ProgressTimer() {
	if (!_verbose) {
		return;
	}
	const uint64_t now = core::TimeProvider::systemMillis();
	const double elapsedSec = (double)(now - _startMs) / 1000.0;
	const int64_t voxels = _voxelStat.load(std::memory_order_relaxed);
	if (voxels > 0) {
		fprintf(stderr, "3dprint %s: TOTAL %.2fs voxels=%ld\n", _tag, elapsedSec, (long)voxels);
		fflush(stderr);
		Log::info("3dprint %s: TOTAL %.2fs voxels=%ld", _tag, elapsedSec, (long)voxels);
	} else {
		fprintf(stderr, "3dprint %s: TOTAL %.2fs\n", _tag, elapsedSec);
		fflush(stderr);
		Log::info("3dprint %s: TOTAL %.2fs", _tag, elapsedSec);
	}
}

void ProgressTimer::addVoxels(int64_t n) {
	_voxelStat.fetch_add(n, std::memory_order_relaxed);
}

void ProgressTimer::emit(int processed, int pct, double elapsedSec, double etaSec, int64_t voxels, bool isFinal) const {
	// Progress lines go to stderr only (no Log::info — avoids the duplicate "INFO:" prefixed line
	// in the voxedit console alongside the raw fprintf output).
	if (isFinal) {
		if (voxels > 0) {
			fprintf(stderr, "3dprint %s: %d/%d (100%%) elapsed %.1fs voxels=%ld (done)\n", _tag, processed,
					_total, elapsedSec, (long)voxels);
		} else {
			fprintf(stderr, "3dprint %s: %d/%d (100%%) elapsed %.1fs (done)\n", _tag, processed, _total,
					elapsedSec);
		}
	} else {
		if (voxels > 0) {
			fprintf(stderr, "3dprint %s: %d/%d (%d%%) elapsed %.1fs eta %.1fs voxels=%ld\n", _tag, processed,
					_total, pct, elapsedSec, etaSec, (long)voxels);
		} else {
			fprintf(stderr, "3dprint %s: %d/%d (%d%%) elapsed %.1fs eta %.1fs\n", _tag, processed, _total, pct,
					elapsedSec, etaSec);
		}
	}
	fflush(stderr);
}

void ProgressTimer::tick(int processed) {
	if (_total <= 0 || !_verbose) {
		return;
	}
	const int pct = (int)((int64_t)processed * 100 / _total);
	const bool isFinal = processed >= _total;

	// Claim this percentage: print only if we move _lastPct forward. For the final tick,
	// also print unconditionally (covers the processed==total case even if pct==lastPct).
	if (!isFinal) {
		int expected = _lastPct.load(std::memory_order_relaxed);
		if (pct <= expected) {
			return;
		}
		while (!_lastPct.compare_exchange_weak(expected, pct, std::memory_order_relaxed,
											   std::memory_order_relaxed)) {
			if (pct <= expected) {
				return;
			}
		}
	}

	const uint64_t now = core::TimeProvider::systemMillis();
	const uint64_t elapsedMs = now - _startMs;
	const double elapsedSec = (double)elapsedMs / 1000.0;
	double etaSec = 0.0;
	if (processed > 0 && processed < _total) {
		// Blend overall average rate with a "recent" rate sampled since the previous
		// emitted tick. Pure overall-average ETA is lazy and wrong when the work pace
		// changes over time (common for sealgaps/fillholes where the tail is faster).
		const uint64_t prevMs = _lastEmittedElapsedMs.load(std::memory_order_relaxed);
		const int prevProcessed = _lastEmittedProcessed.load(std::memory_order_relaxed);
		const int deltaProcessed = processed - prevProcessed;
		const int64_t deltaMs = (int64_t)elapsedMs - (int64_t)prevMs;
		double recentRatePerSec = 0.0;
		if (deltaProcessed > 0 && deltaMs > 0) {
			recentRatePerSec = (double)deltaProcessed * 1000.0 / (double)deltaMs;
		}
		const double overallRatePerSec = (double)processed / elapsedSec;
		// 75% weight to recent rate (if available), 25% to overall — tracks pace changes
		// without being too jumpy from single-tick noise.
		const double blendedRate = (recentRatePerSec > 0.0)
										? (0.75 * recentRatePerSec + 0.25 * overallRatePerSec)
										: overallRatePerSec;
		if (blendedRate > 0.0) {
			etaSec = (double)(_total - processed) / blendedRate;
		}
	}
	_lastEmittedElapsedMs.store(elapsedMs, std::memory_order_relaxed);
	_lastEmittedProcessed.store(processed, std::memory_order_relaxed);
	const int64_t voxels = _voxelStat.load(std::memory_order_relaxed);
	emit(processed, pct, elapsedSec, etaSec, voxels, isFinal);
}

} // namespace printing
} // namespace voxedit
