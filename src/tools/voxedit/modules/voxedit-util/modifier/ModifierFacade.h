/**
 * @file
 */

#pragma once

#include "IModifierRenderer.h"
#include "Modifier.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"

namespace voxedit {

class ModifierFacade : public Modifier {
private:
	using Super = Modifier;
	ModifierRendererPtr _modifierRenderer;
	SceneManager *_sceneMgr;

	// Preview volume management - the facade owns these and prepares them for the renderer
	core::ScopedPtr<voxel::RawVolume> _previewMirrorVolume;
	core::ScopedPtr<voxel::RawVolume> _previewVolume;
	core::VarPtr _maxSuggestedVolumeSizePreview;
	double _nextPreviewUpdateSeconds = 0;

	bool previewNeedsExistingVolume() const;
	void updateBrushVolumePreview(palette::Palette &activePalette, ModifierRendererContext &ctx);
	bool isSimplePreview(const Brush *brush, const voxel::Region &region) const;

public:
	ModifierFacade(SceneManager *sceneMgr, const ModifierRendererPtr &modifierRenderer);
	bool init() override;
	void shutdown() override;
	void render(const video::Camera &camera, palette::Palette &activePalette,
				const glm::mat4 &model = glm::mat4(1.0f));
};

} // namespace voxedit
