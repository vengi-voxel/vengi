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
	SceneManager *_sceneMgr;

	void updateBrushVolumePreview(palette::Palette &palette);

public:
	ModifierFacade(SceneManager *sceneMgr, const ModifierRendererPtr &modifierRenderer);
	bool init() override;
	void shutdown() override;
	void render(const video::Camera &camera, palette::Palette &palette);
};

} // namespace voxedit
