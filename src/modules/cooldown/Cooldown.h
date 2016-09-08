/**
 * @file
 */

#pragma once

#include "core/TimeProvider.h"
#include "CooldownType.h"
#include "CooldownTriggerState.h"

#include <memory>

namespace cooldown {

class Cooldown {
private:
	Type _type;
	unsigned long _durationMillis;
	unsigned long _startMillis;
	unsigned long _expireMillis;
	core::TimeProviderPtr _timeProvider;

public:
	Cooldown(Type type, unsigned long durationMillis, const core::TimeProviderPtr& timeProvider) :
			_type(type), _durationMillis(durationMillis), _startMillis(0ul), _expireMillis(0ul), _timeProvider(timeProvider) {
	}

	inline void start() {
		_startMillis = _timeProvider->tickTime();
		_expireMillis = _startMillis + _durationMillis;
	}

	inline void reset() {
		_startMillis = 0ul;
		_expireMillis = 0ul;
	}

	inline void expire() {
		// TODO: eventbus or state
		reset();
	}

	inline void cancel() {
		// TODO: eventbus or state
		reset();
	}

	unsigned long durationMillis() const {
		return _durationMillis;
	}

	inline bool started() const {
		return _expireMillis > 0ul;
	}

	inline bool running() const {
		return _expireMillis > 0ul && _timeProvider->tickTime() < _expireMillis;
	}

	inline unsigned long duration() const {
		return _expireMillis - _startMillis;
	}

	inline unsigned long startMillis() const {
		return _startMillis;
	}

	inline Type type() const {
		return _type;
	}

	inline bool operator<(const Cooldown& rhs) const {
		return _expireMillis < rhs._expireMillis;
	}
};

typedef std::shared_ptr<Cooldown> CooldownPtr;

}

namespace std {
template<> struct hash<cooldown::Cooldown> {
	inline size_t operator()(const cooldown::Cooldown &c) const {
		return std::hash<size_t>()(static_cast<size_t>(c.type()));
	}
};
}
