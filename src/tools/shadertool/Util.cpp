/**
 * @file
 */

#include "Util.h"
#include "core/String.h"
#include <vector>
#include <SDL.h>

namespace util {

std::string convertName(const std::string& in, bool firstUpper) {
	std::string out;
	std::vector<std::string> nameParts;
	core::string::splitString(in, nameParts, "_");
	for (std::string& n : nameParts) {
		if (n.length() > 1 || nameParts.size() < 2) {
			if (!firstUpper) {
				firstUpper = true;
			} else {
				n[0] = SDL_toupper(n[0]);
			}
			out += n;
		}
	}
	if (out.empty()) {
		out = in;
	}
	return out;
}


}
