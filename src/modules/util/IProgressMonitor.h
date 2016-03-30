#pragma once

namespace util {

class IProgressMonitor {
protected:
	long _max;
	long _steps;
public:
	IProgressMonitor(long max = 100l):
			_max(max), _steps(0l) {
	}

	virtual ~IProgressMonitor() {
	}

	void init(long max) {
		_max = max;
	}

	virtual void step(long steps = 1l) {
		_steps += steps;
	}

	virtual void done() {
	}

	double progress() const {
		return _steps * 100.0 / static_cast<double>(_max);
	}
};

}
