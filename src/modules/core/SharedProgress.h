/**
 * @file
 */

#pragma once

#include "core/IProgress.h"
#include "core/String.h"
#include "core/Trace.h"
#include "core/concurrent/Lock.h"

namespace core {

/**
 * @brief Thread-safe @c IProgress sink for async work (e.g. voxedit file load).
 *
 * Writers call setProgress/setText from worker threads; the UI reads progress()/text()
 * on the main thread.
 */
class SharedProgress : public IProgress {
private:
	mutable core_trace_mutex(core::Lock, _lock, "SharedProgress");
	float _value = 0.0f;
	core::String _text;

public:
	void reset() {
		ScopedLock lock(_lock);
		_value = 0.0f;
		_text.clear();
	}

	void setProgress(float value) override {
		ScopedLock lock(_lock);
		_value = clamp01(value);
	}

	void setText(const char *text) override {
		ScopedLock lock(_lock);
		if (text == nullptr) {
			_text.clear();
			return;
		}
		_text = text;
	}

	float progress() const {
		ScopedLock lock(_lock);
		return _value;
	}

	const core::String &text() const {
		ScopedLock lock(_lock);
		return _text;
	}
};

} // namespace core
