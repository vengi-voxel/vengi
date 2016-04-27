#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include "String.h"

namespace ai {

class IParser {
private:
	std::string _error;

protected:
	void setError(const std::string& error);

	inline std::string getBetween (const std::string& str, const std::string& tokenStart, const std::string& tokenEnd) {
		const std::size_t start = str.find(tokenStart);
		if (start == std::string::npos)
			return "";

		const std::size_t end = str.find(tokenEnd);
		if (end == std::string::npos) {
			setError("syntax error - expected " + tokenEnd);
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

inline void IParser::setError(const std::string& error) {
	_error = error;
}

inline const std::string& IParser::getError() const {
	return _error;
}

}
