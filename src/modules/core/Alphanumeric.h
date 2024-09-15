/**
 * @file
 */

#include "String.h"
#include "core/StringUtil.h"
#include <ctype.h>

namespace core {

class Alphanumeric {
private:
	const char *_value;

public:
	Alphanumeric(const char *str) : _value(str) {
	}

	// Overloading the < operator
	bool operator<(const Alphanumeric &other) const {
		size_t i = 0, j = 0;
		size_t len1 = strlen(_value);
		size_t len2 = strlen(other._value);

		while (i < len1 && j < len2) {
			if (isdigit(_value[i]) && isdigit(other._value[j])) {
				// Compare numeric values
				core::String num1, num2;

				// Extract full number from the first string
				while (i < len1 && isdigit(_value[i])) {
					num1 += _value[i++];
				}

				// Extract full number from the second string
				while (j < len2 && isdigit(other._value[j])) {
					num2 += other._value[j++];
				}

				// Compare the numerical values as integers
				int int1 = core::string::toInt(num1);
				int int2 = core::string::toInt(num2);
				if (int1 != int2) {
					return int1 < int2;
				}
			} else if (isalpha(_value[i]) && isalpha(other._value[j])) {
				// Compare alphabetically
				char v1 = string::toLower(_value[i]);
				char v2 = string::toLower(other._value[j]);
				if (v1 != v2) {
					return v1 < v2;
				}
				++i;
				++j;
			} else {
				// Digits come before letters
				return isdigit(_value[i]) && isalpha(other._value[j]);
			}
		}

		// If we reached here, one string is a prefix of the other
		return len1 < len2;
	}

	bool operator>(const Alphanumeric &other) const {
		return other < *this;
	}

	const char *c_str() const {
		return _value;
	}
};

} // namespace core
