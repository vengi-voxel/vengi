/**
 * @file
 */

#pragma once

#include "core/IComponent.h"

namespace core {

class DeltaFrameSeconds : public IComponent {
protected:
	double _deltaSeconds = -1.0;
	double _nowSeconds = 0.0;

public:
	virtual ~DeltaFrameSeconds() {
	}

	void updateDelta(double nowSeconds) {
		if (_deltaSeconds < 0.0) {
			_deltaSeconds = 0.0;
		} else {
			_deltaSeconds = nowSeconds - _nowSeconds;
		}
		_nowSeconds = nowSeconds;
	}

	double deltaSeconds() const {
		return _deltaSeconds;
	}

	double nowSeconds() const {
		return _nowSeconds;
	}
};

} // namespace core
