#include "Timestamp.h"

namespace persistence {

// TODO: POSTGRES: microseconds
Timestamp::Timestamp(uint64_t time) :
		_time(time), _now(false) {
}

Timestamp::Timestamp() :
	Timestamp(0L) {
}

Timestamp Timestamp::now() {
	static Timestamp nowInstance;
	nowInstance._now = true;
	return nowInstance;
}

bool Timestamp::isNow() const {
	return _now;
}

uint64_t Timestamp::time() const {
	return _time;
}

}
