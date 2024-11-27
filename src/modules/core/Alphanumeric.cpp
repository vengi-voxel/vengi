/**
 * @file
 */

#include "Alphanumeric.h"
#include "UTF8.h"
#include "core/StandardLib.h"
#include <ctype.h>
#include <string.h>

namespace core {

int Alphanumeric::ndigits(const char *s) {
	const int n = utf8::length(s);

	int nd = 0;
	for (int i = 0; i < n; i++) {
		int c = utf8::next(&s);
		if (c < 0) {
			break;
		}
		if (c >= '0' && c <= '9') {
			++nd;
		}
	}
	return nd;
}

bool Alphanumeric::operator<(const Alphanumeric &other) const {
	const char *s = _value;
	const char *t = other._value;

	const int sd = ndigits(s);
	const int td = ndigits(t);

	/* presence of digits */
	if (!sd && !td) {
		return core_strcasecmp(s, t) < 0;
	}
	if (!sd) {
		return false;
	}
	if (!td) {
		return true;
	}

	/* value of digits */
	int i, j;
	for (i = 0, j = 0; i < sd && j < td; ++i, ++j) {
		while (!isdigit(*s)) {
			++s;
		}
		while (!isdigit(*t)) {
			++t;
		}

		if (*s != *t) {
			return *s - *t;
		}
		++s;
		++t;
	}

	/* number of digits */
	if (i < sd) {
		return false;
	}
	if (j < td) {
		return true;
	}

	/* value of string after last digit */
	return core_strcasecmp(s, t) < 0;
}

} // namespace core
