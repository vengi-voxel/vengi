/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"

/**
 * @brief Adds the options (dependent on the mode) for the given @c io::FormatDescription instances to the file dialog
 */
extern void fileDialogOptions(video::OpenFileMode mode, const io::FormatDescription *desc);
