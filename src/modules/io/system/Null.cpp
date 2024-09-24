/**
 * @file
 */

#include <SDL_platform.h>

#if !defined(__LINUX__) && !defined(__MACOSX__) && !defined(__WINDOWS__) && !defined(__EMSCRIPTEN__)
#include "io/Filesystem.h"

namespace io {

bool initState(io::FilesystemState &state) {
	return false;
}

bool fs_mkdir(const core::Path &path) {
	return false;
}

bool fs_rmdir(const core::Path &path) {
	return false;
}

bool fs_hidden(const core::Path &path) {
	return false;
}

bool fs_unlink(const core::Path &path) {
	return false;
}

bool fs_exists(const core::Path &path) {
	return false;
}

bool fs_writeable(const core::Path &path) {
	return false;
}

bool fs_chdir(const core::Path &path) {
	return false;
}

core::Path fs_realpath(const core::Path &path) {
	return path;
}

bool fs_stat(const core::Path &path, FilesystemEntry &entry) {
	return false;
}

core::DynamicArray<FilesystemEntry> fs_scandir(const core::Path &path) {
	core::DynamicArray<FilesystemEntry> foo;
	return foo;
}

core::Path fs_readlink(const core::Path &path) {
	return path;
}

core::Path fs_cwd() {
	return core::Path("/");
}

} // namespace io

#endif
