/**
 * @file
 */
#pragma once

#include "Log.h"
#include <vector>
#include <string>
#include <algorithm>
#include "String.h"

namespace ai {

class IParser {
private:
	std::string _error;

protected:
	void setError(const char* msg, ...) __attribute__((format(printf, 2, 3)));

	inline void resetError() {
		_error = "";
	}

	inline std::string getBetween (const std::string& str, const std::string& tokenStart, const std::string& tokenEnd) {
		const std::size_t start = str.find(tokenStart);
		if (start == std::string::npos) {
			return "";
		}

		const std::size_t end = str.find(tokenEnd);
		if (end == std::string::npos) {
			setError("syntax error - expected %s", tokenEnd.c_str());
			return "";
		}
		const size_t startIndex = start + 1;
		const size_t endIndex = end - startIndex;
		if (endIndex <= 0) {
			return "";
		}
		const std::string& between = str.substr(startIndex, endIndex);
		return between;
	}

public:
	const std::string& getError() const;
};

inline void IParser::setError(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[1024];
	std::vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	_error = buf;
}


inline const std::string& IParser::getError() const {
	return _error;
}

}
