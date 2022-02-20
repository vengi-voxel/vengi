/**
 * @file
 */

#include <SDL_platform.h>

#if !defined(__LINUX__) && !defined(__MACOSX__) && !defined(__WINDOWS__)
#include "io/Filesystem.h"

namespace io {

bool initState(io::FilesystemState &state) {
	return false;
}

} // namespace io

#endif
