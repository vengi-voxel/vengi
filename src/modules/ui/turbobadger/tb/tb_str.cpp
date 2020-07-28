/**
 * @file
 */

#include "tb_str.h"
#include "core/Assert.h"

namespace tb {

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
