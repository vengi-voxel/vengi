#pragma once

#include "core/String.h"
#include <regex>

namespace util {

static inline bool isValidEmail(const std::string& email) {
	if (email.size() > 256) {
		return false;
	}
	static std::regex r("^[_a-z0-9-]+(\\.[_a-z0-9-]+)*@[a-z0-9-]+(\\.[a-z0-9-]+)*(\\.[a-z]{2,4})$",
			std::regex::extended | std::regex::icase | std::regex::optimize);
	return std::regex_search(email, r);
}

}
