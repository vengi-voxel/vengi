/**
 * @file
 */

#include "Timestamp.h"

namespace persistence {

// TODO: POSTGRES: microseconds
Timestamp::Timestamp(uint64_t millis) :
		_millis(millis), _now(false) {
}

Timestamp::Timestamp() :
	Timestamp(0L) {
}

Timestamp Timestamp::now() {
	static Timestamp nowInstance;
	nowInstance._now = true;
	return nowInstance;
}

}
