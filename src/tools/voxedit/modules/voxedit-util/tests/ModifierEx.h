/**
 * @file
 */

#pragma once

#include "../modifier/Modifier.h"
#include "../modifier/PreviewManager.h"

namespace voxedit {

/**
 * @brief Test helper that exposes internal Modifier methods which are not part of the public API
 * but are needed for unit testing preview functionality.
 */
class ModifierEx : public Modifier {
public:
	using Modifier::Modifier;

	void resetPreview() {
		_previewManager.resetPreview();
	}

	const BrushPreview &brushPreview() const {
		return _previewManager.brushPreview();
	}

	voxel::RawVolume *previewVolume() const {
		return _previewManager.previewVolume();
	}

	voxel::RawVolume *previewMirrorVolume() const {
		return _previewManager.previewMirrorVolume();
	}
};

} // namespace voxedit
