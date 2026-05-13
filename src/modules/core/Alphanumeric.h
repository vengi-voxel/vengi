/**
 * @file
 */

namespace core {

/**
 * @brief Compare strings alphanumerically.
 *
 * This class is used to wrap a C-string and provide an overloaded `<` operator
 * that performs an alphanumeric string comparison.
 *
 * Perform 'natural order' comparisons of strings
 */
class Alphanumeric {
private:
	const char *_value;

public:
	Alphanumeric(const char *str) : _value(str) {
	}

	// Overloading the < operator
	// https://stackoverflow.com/a/10204043/774082
	bool operator<(const Alphanumeric &other) const;

	bool operator>(const Alphanumeric &other) const {
		return other < *this;
	}

	const char *c_str() const {
		return _value;
	}
};

} // namespace core
