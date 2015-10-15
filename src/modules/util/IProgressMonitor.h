#pragma once

namespace util {

class IProgressMonitor {
protected:
	int _max;
	int _steps;
public:
	IProgressMonitor(int max = 100) :
			_max(max), _steps(0) {
	}

	virtual ~IProgressMonitor() {
	}

	void init(int max) {
		_max = max;
	}

	virtual void step(int steps = 1) {
		_steps += steps;
	}

	virtual void done() {
	}

	double progress() const {
		return _steps * 100.0 / static_cast<double>(_max);
	}
};

}
