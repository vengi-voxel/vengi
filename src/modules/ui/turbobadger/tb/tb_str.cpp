/**
 * @file
 */

#include "tb_str.h"
#include "core/Assert.h"

namespace tb {

static const char *empty = "";
inline void safe_delete(char *&str) {
	if (str != empty && (str != nullptr)) {
		SDL_free(str);
	}
	str = const_cast<char *>(empty);
}

const char *stristr(const char *arg1, const char *arg2) {
	const char *a;
	const char *b;

	for (; *arg1 != 0; arg1++) {
		a = arg1;
		b = arg2;
		while (SDL_toupper(*a++) == SDL_toupper(*b++)) {
			if (*b == 0) {
				return arg1;
			}
		}
	}
	return nullptr;
}

} // namespace tb
