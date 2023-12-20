/**
 * @file
 */

#pragma once

#include "Modifier.h"
#include "IModifierRenderer.h"
#include "core/ScopedPtr.h"

namespace voxedit {

class ModifierFacade : public Modifier {
private:
	using Super = Modifier;
	ModifierRendererPtr _modifierRenderer;
	core::ScopedPtr<voxel::RawVolume> _mirrorVolume;
	core::ScopedPtr<voxel::RawVolume> _volume;
	void updateBrushVolumePreview(palette::Palette &palette);

public:
	ModifierFacade(const ModifierRendererPtr &modifierRenderer);
	bool init() override;
	void shutdown() override;
	void render(const video::Camera &camera, palette::Palette &palette);
};

} // namespace voxedit
