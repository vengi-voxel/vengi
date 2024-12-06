/**
 * @file
 */

#include "Alphanumeric.h"
#include "external/strnatcmp.h"

namespace core {

bool Alphanumeric::operator<(const Alphanumeric &other) const {
	return strnatcasecmp(_value, other._value) < 0;
}

} // namespace core
