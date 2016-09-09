#pragma once

#include <string>
#include <regex>

namespace util {

static inline bool isValidEmail(const std::string& email) {
	if (email.size() > 256) {
		return false;
	}
	static std::regex r("^[_a-z0-9-]+(\\.[_a-z0-9-]+)*@[a-z0-9-]+(\\.[a-z0-9-]+)*(\\.[a-z]{2,4})$", std::regex::extended | std::regex_constants::icase);
	return std::regex_search(email, r);
}

}
