/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"

/**
 * @brief Adds the options (dependent on the mode) for the given @c io::FormatDescription instances to the file dialog
 */
bool fileDialogOptions(video::OpenFileMode mode, const io::FormatDescription *desc, const io::FilesystemEntry &entry);

bool genericOptions(const io::FormatDescription *desc);
bool saveOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry);
bool loadOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry);
