/**
 * @file
 */

#pragma once

#include "Modifier.h"
#include "IModifierRenderer.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"

namespace voxedit {

class ModifierFacade : public Modifier {
private:
	using Super = Modifier;
	ModifierRendererPtr _modifierRenderer;
	core::ScopedPtr<voxel::RawVolume> _previewMirrorVolume;
	core::ScopedPtr<voxel::RawVolume> _previewVolume;
	SceneManager *_sceneMgr;
	core::VarPtr _maxSuggestedVolumeSizePreview;
	double _nextPreviewUpdateSeconds = 0;

	bool previewNeedsExistingVolume() const;
	void updateBrushVolumePreview(palette::Palette &activePalette);
	/**
	 * @brief Should the simple preview rendering be used
	 * @param brush The brush to use
	 * @param region The region to use
	 * @return true if the simple preview cube-based rendering should be used
	 * @return false if the complex preview voxel-based rendering should be used
	 */
	bool generateSimplePreview(const Brush *brush, const voxel::Region &region) const;
	void handleSelection(const video::Camera &camera, const glm::mat4 &model, Brush *brush);

public:
	ModifierFacade(SceneManager *sceneMgr, const ModifierRendererPtr &modifierRenderer, const SelectionManagerPtr &selectionManager);
	bool init() override;
	void shutdown() override;
	void render(const video::Camera &camera, palette::Palette &activePalette, const glm::mat4 &model = glm::mat4(1.0f));
};

} // namespace voxedit
