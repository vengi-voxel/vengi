/**
 * @brief
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace io {
class Filesystem;
using FilesystemPtr = core::SharedPtr<Filesystem>;
} // namespace io

namespace palette {

/**
 * @brief Searches available palettes
 */
class PaletteCache {
private:
	io::FilesystemPtr _filesystem;
	core::DynamicArray<core::String> _availablePalettes;

public:
	PaletteCache(const io::FilesystemPtr &filesystem) : _filesystem(filesystem) {
	}

	void clear();
	virtual void detectPalettes(bool includeBuiltIn = true);
	void add(const core::String &paletteName);
	const core::DynamicArray<core::String> &availablePalettes() const;
};

} // namespace palette
