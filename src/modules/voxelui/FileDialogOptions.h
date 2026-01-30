/**
 * @file
 */

#pragma once

namespace palette {
class PaletteCache;
}

#include "video/WindowedApp.h"

namespace voxelui {

/**
 * @brief Adds the options (dependent on the mode) for the given @c io::FormatDescription instances to the file dialog
 */
class FileDialogOptions {
private:
	palette::PaletteCache &_paletteCache;
	const bool _palette;

public:
	FileDialogOptions(palette::PaletteCache &paletteCache, bool palette);
	bool operator()(video::OpenFileMode mode, const io::FormatDescription *desc, const io::FilesystemEntry &entry);
	static video::FileDialogOptions build(palette::PaletteCache &paletteCache, bool palette);
};

// palette options
bool paletteOptions(video::OpenFileMode mode, const io::FormatDescription *desc);

// voxel format options
bool genericOptions(const io::FormatDescription *desc);
bool saveOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry);
bool loadOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry,
				 const palette::PaletteCache &paletteCache);
// mesh mode for exporting/saving meshes - includes greedy texture
void meshModeOption();

} // namespace voxelui
