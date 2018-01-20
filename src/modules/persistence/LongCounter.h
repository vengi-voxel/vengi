/**
 * @file
 */

#pragma once

namespace persistence {

/**
 * @brief This counter maintains a state between the value that is known in the database and the
 * value that is current in the memory.
 *
 * @c update() will deliver the delta value that should be used to do a relative update on the database.
 * Just to persist the delta.
 */
class LongCounter {
private:
	long _current;
	long _persisted;
public:
	LongCounter(long initial = 0L, long persisted = 0L) :
			_current(initial), _persisted(persisted) {
	}

	/**
	 * @return The delta between the value that is persisted and the value that is currently in memory.
	 */
	long update() {
		const long c = _current;
		_current = _persisted;
		return c - _persisted;
	}

	/**
	 * @return The current value as in memory
	 */
	inline long value() const {
		return _current;
	}
};

}
