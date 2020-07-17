/**
 * @file
 */
#pragma once

#include "core/Common.h"
#include "core/String.h"

namespace backend {

class IParser {
private:
	core::String _error;

protected:
	void setError(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);

	inline void resetError() {
		_error = "";
	}

	core::String getBetween(const core::String& str, const core::String& tokenStart, const core::String& tokenEnd);

public:
	const core::String& getError() const;
};

inline const core::String& IParser::getError() const {
	return _error;
}

}
