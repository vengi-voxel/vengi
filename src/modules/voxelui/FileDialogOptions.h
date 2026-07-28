/**
 * @file
 */

#pragma once

namespace palette {
class PaletteCache;
}

#include "video/WindowedApp.h"

namespace voxelui {

class ScenePreview;

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
	/**
	 * @param preview Optional long-lived @c ScenePreview used as a side panel in the file dialog
	 *                (voxel open/save dialogs only; ignored for palette mode).
	 */
	static video::FileDialogOptions build(palette::PaletteCache &paletteCache, bool palette,
										  ScenePreview *preview = nullptr);
};

// palette options
bool paletteOptions(video::OpenFileMode mode, const io::FormatDescription *desc);

// voxel format options
bool saveOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry);
bool loadOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry,
				 const palette::PaletteCache &paletteCache);
// mesh mode for exporting/saving meshes - includes greedy texture
void meshModeOption();

} // namespace voxelui
