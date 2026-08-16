/**
 * @file
 */

#include "core/ProgressScope.h"

namespace core {

namespace {
AtomicPtr<IProgress> g_currentProgress;
}

IProgress *currentProgressPtr() {
	return g_currentProgress;
}

IProgress &currentProgress() {
	if (IProgress *progress = g_currentProgress) {
		return *progress;
	}
	return NullProgress::get();
}

IProgress *setCurrentProgress(IProgress *progress) {
	return g_currentProgress.exchange(progress);
}

ProgressScope::ProgressScope(IProgress &progress) : _previous(setCurrentProgress(&progress)) {
}

ProgressScope::~ProgressScope() {
	setCurrentProgress(_previous);
}

} // namespace core
