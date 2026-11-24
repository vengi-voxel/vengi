/**
 * @file
 */

#pragma once

#include "core/Path.h"
#include "core/collection/DynamicArray.h"
#include "io/FilesystemEntry.h"

namespace io {

bool fs_mkdir(const core::Path &path);
bool fs_rmdir(const core::Path &path);
bool fs_unlink(const core::Path &path);
bool fs_exists(const core::Path &path);
bool fs_writeable(const core::Path &path);
bool fs_hidden(const core::Path &path);
bool fs_chdir(const core::Path &path);
core::Path fs_realpath(const core::Path &path);
bool fs_stat(const core::Path &path, FilesystemEntry &entry);
core::DynamicArray<FilesystemEntry> fs_scandir(const core::Path &path);
core::Path fs_readlink(const core::Path &path);
core::Path fs_cwd();

} // namespace io
