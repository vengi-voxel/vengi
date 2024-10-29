/**
 * @file
 */

#include <ctype.h>
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

namespace core {

class Alphanumeric {
private:
	const char *_value;

	static int ndigits(const char *s) {
		const int n = strlen(s);

		int nd = 0;
		for (int i = 0; i < n; i++) {
			if (isdigit(s[i])) {
				++nd;
			}
		}
		return nd;
	}

public:
	Alphanumeric(const char *str) : _value(str) {
	}

	// Overloading the < operator
	// https://stackoverflow.com/a/10204043/774082
	bool operator<(const Alphanumeric &other) const {
		const char *s = _value;
		const char *t = other._value;

		const int sd = ndigits(s);
		const int td = ndigits(t);

		/* presence of digits */
		if (!sd && !td) {
			return strcasecmp(s, t);
		}
		if (!sd) {
			return 1;
		}
		if (!td) {
			return -1;
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
			return 1;
		}
		if (j < td) {
			return -1;
		}

		/* value of string after last digit */
		return strcasecmp(s, t);
	}

	bool operator>(const Alphanumeric &other) const {
		return other < *this;
	}

	const char *c_str() const {
		return _value;
	}
};

} // namespace core
