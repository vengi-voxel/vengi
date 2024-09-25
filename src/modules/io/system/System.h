/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"

namespace io {

bool fs_mkdir(const char *path);
bool fs_rmdir(const char *path);
bool fs_unlink(const char *path);
bool fs_exists(const char *path);
bool fs_writeable(const char *path);
bool fs_hidden(const char *path);
bool fs_chdir(const char *path);
core::String fs_realpath(const char *path);
bool fs_stat(const char *path, FilesystemEntry &entry);
core::DynamicArray<FilesystemEntry> fs_scandir(const char *path);
core::String fs_readlink(const char *path);
core::String fs_cwd();

} // namespace io
