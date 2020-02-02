/**
 *
 */

#include "EMailValidator.h"
#include <regex>

namespace util {

bool isValidEmail(const core::String& email) {
	if (email.size() > 256) {
		return false;
	}
	static std::regex r("^[_a-z0-9-]+(\\.[_a-z0-9-]+)*@[a-z0-9-]+(\\.[a-z0-9-]+)*(\\.[a-z]{2,4})$",
			std::regex::extended | std::regex::icase | std::regex::optimize);
	return std::regex_search(email.c_str(), r);
}

}
