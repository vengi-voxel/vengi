/**
 * @file
 */

#pragma once

#include <atomic>
#include <stdint.h>

namespace voxedit {
namespace printing {

// Scoped progress + timing helper. On construction logs "starting"; tick() logs at 1%
// granularity (thread-safe) with elapsed and ETA; destructor logs TOTAL wall time so
// a single `ProgressTimer t("regrid", N);` at function scope gives both per-1% ETA and
// an end-of-command total line.
//
// Pass verbose=false to suppress all output (the timer still tracks state for
// callers that read addVoxels()/tick() side effects, but emits nothing). Used
// by 3dprint holefill when its kHolefillVerbose toggle is off.
class ProgressTimer {
private:
	const char *_tag;
	int _total;
	uint64_t _startMs;
	bool _verbose;
	std::atomic<int> _lastPct{-1};
	// Timestamp (ms since _startMs) and processed count captured at the last tick that
	// actually emitted a log line. Used to compute a "recent" rate so the ETA weights
	// the tail of the run more than the overall average; important for jobs whose pace
	// accelerates or decelerates (typical of 3dprint sealgaps/fillholes).
	std::atomic<uint64_t> _lastEmittedElapsedMs{0};
	std::atomic<int> _lastEmittedProcessed{0};
	// Optional per-command voxel stat: callers increment via addVoxels(n). Displayed in
	// every progress tick and in the final TOTAL summary. Meaning is command-specific
	// (sealed voxels / filled voxels / written voxels / etc).
	std::atomic<int64_t> _voxelStat{0};

	void emit(int processed, int pct, double elapsedSec, double etaSec, int64_t voxels, bool isFinal) const;

public:
	ProgressTimer(const char *tag, int total, bool verbose = true);
	ProgressTimer(const ProgressTimer &) = delete;
	ProgressTimer &operator=(const ProgressTimer &) = delete;
	~ProgressTimer();

	void tick(int processed);
	void addVoxels(int64_t n);
};

} // namespace printing
} // namespace voxedit
