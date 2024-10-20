/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"

/**
 * @brief Adds the options (dependent on the mode) for the given @c io::FormatDescription instances to the file dialog
 */
void fileDialogOptions(video::OpenFileMode mode, const io::FormatDescription *desc, const io::FilesystemEntry &entry);

void genericOptions(const io::FormatDescription *desc, bool targetFileExists, bool &overwriteTargetFile);
void targetOptions(const io::FormatDescription *desc);
void sourceOptions(const io::FormatDescription *desc);
