/**
 * @file
 */

#pragma once

#include "core/Common.h"

namespace core {

/**
 * @brief Normalized progress sink in the range [0, 1].
 *
 * Keep this interface free of algorithm-specific concepts (steps, triangles, nodes).
 * Compose hierarchy with @c ProgressRange / @c StepProgress on top.
 */
class IProgress {
public:
	virtual ~IProgress() {
	}

	virtual void setProgress(float value) = 0;
	virtual void setText(const char * /*text*/) {
	}
};

class NullProgress : public IProgress {
public:
	void setProgress(float /*value*/) override {
	}

	static NullProgress &get() {
		static NullProgress instance;
		return instance;
	}
};

inline IProgress &progressOrNull(IProgress *progress) {
	if (progress != nullptr) {
		return *progress;
	}
	return NullProgress::get();
}

inline float clamp01(float value) {
	if (value < 0.0f) {
		return 0.0f;
	}
	if (value > 1.0f) {
		return 1.0f;
	}
	return value;
}

/**
 * @brief Maps local [0,1] progress into a sub-range of a parent @c IProgress.
 *
 * Nest freely: parent 0..1, child 0.2..0.5, grandchild 0..1 of that child, etc.
 */
class ProgressRange : public IProgress {
private:
	IProgress &_parent;
	float _begin;
	float _end;

public:
	ProgressRange(IProgress &parent, float begin, float end) : _parent(parent), _begin(begin), _end(end) {
	}

	void setProgress(float value) override {
		value = clamp01(value);
		_parent.setProgress(_begin + value * (_end - _begin));
	}

	void setText(const char *text) override {
		_parent.setText(text);
	}
};

/**
 * @brief Equal-weight steps over a parent progress range.
 *
 * @code
 * StepProgress steps(progress, 5);
 * ProgressRange step0 = steps.range(0); // 0.0 .. 0.2
 * step0.setProgress(0.5f);              // global 0.1
 * @endcode
 */
class StepProgress {
private:
	IProgress &_progress;
	int _steps;

public:
	StepProgress(IProgress &progress, int steps) : _progress(progress), _steps(core_max(1, steps)) {
	}

	int steps() const {
		return _steps;
	}

	IProgress &progress() {
		return _progress;
	}

	/**
	 * @param step Zero-based step index in [0, steps)
	 * @param subProgress Progress inside the step in [0, 1]
	 */
	void report(int step, float subProgress) {
		if (step < 0) {
			step = 0;
		}
		if (step >= _steps) {
			step = _steps - 1;
		}
		subProgress = clamp01(subProgress);
		_progress.setProgress(((float)step + subProgress) / (float)_steps);
	}

	ProgressRange range(int step) {
		if (step < 0) {
			step = 0;
		}
		if (step >= _steps) {
			step = _steps - 1;
		}
		const float begin = (float)step / (float)_steps;
		const float end = (float)(step + 1) / (float)_steps;
		return ProgressRange(_progress, begin, end);
	}
};

} // namespace core
