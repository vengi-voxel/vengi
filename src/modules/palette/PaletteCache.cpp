/**
 * @brief
 */

#include "PaletteCache.h"
#include "io/Filesystem.h"
#include "palette/Palette.h"

namespace palette {

void PaletteCache::clear() {
	_availablePalettes.clear();
}

void PaletteCache::detectPalettes(bool includeBuiltIn) {
	core::DynamicArray<io::FilesystemEntry> entities;
	_filesystem->list("", entities, "palette-*.png");
	for (const io::FilesystemEntry &file : entities) {
		if (file.type != io::FilesystemEntry::Type::file) {
			continue;
		}
		const core::String &name = Palette::extractPaletteName(file.name);
		_availablePalettes.push_back(name);
	}

	if (includeBuiltIn) {
		for (int i = 0; i < lengthof(palette::Palette::builtIn); ++i) {
			_availablePalettes.push_back(palette::Palette::builtIn[i]);
		}
	}
}

void PaletteCache::add(const core::String &paletteName) {
	_availablePalettes.push_back(paletteName);
}

const core::DynamicArray<core::String> &PaletteCache::availablePalettes() const {
	return _availablePalettes;
}

} // namespace palette
