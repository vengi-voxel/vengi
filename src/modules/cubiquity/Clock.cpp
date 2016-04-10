#include "Clock.h"

#include "PolyVox/ErrorHandling.h"
#include "core/Common.h"

#include <limits>

namespace Cubiquity {

// We initialise the clock to a reasonably sized value, so that we can initialise
// timestamps to small values and be sure that they will immediatly be out-of-date.
Timestamp Clock::_timestamp = 100;

Timestamp Clock::getTimestamp(void) {
	// I don't think we need to protect this operation with a mutex. Potentially two threads could enter this function and then
	// leave in a different order to which they entered, but I don't think that matters as long as the timestamps are unique?
	core_assert_msg(_timestamp < (std::numeric_limits<Timestamp>::max)(), "Time stamp is wrapping around.");
	return ++_timestamp;
}

}
