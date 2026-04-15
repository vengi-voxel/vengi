/**
 * @file
 */

#pragma once

#include "IModifierRenderer.h"
#include "SceneModifiedFlags.h"
#include "brush/Brush.h"
#include "brush/BrushType.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "color/RGBA.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"

namespace palette {
class Palette;
}

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
}

namespace voxedit {

class Modifier;

/**
 * @brief Stores the result of brush preview generation
 *
 * This contains the computed preview state that can be used by renderers
 * or tests to verify preview behavior without requiring a renderer.
 */
struct BrushPreview {
	voxel::Region simplePreviewRegion = voxel::Region::InvalidRegion;
	voxel::Region simpleMirrorPreviewRegion = voxel::Region::InvalidRegion;
	color::RGBA simplePreviewColor{0};
	bool useSimplePreview = false;
};

/**
 * @brief Manages brush preview volume lifecycle, throttling, and simple vs. volume detection
 */
class PreviewManager {
private:
	ModifierRendererPtr _modifierRenderer;
	core::ScopedPtr<voxel::RawVolume> _previewVolume;
	core::ScopedPtr<voxel::RawVolume> _previewMirrorVolume;
	core::VarPtr _maxSuggestedVolumeSizePreview;
	BrushPreview _brushPreview;
	double _nextPreviewUpdateSeconds = 0;

	/**
	 * @brief Check if the current brush/modifier combination needs an existing volume
	 * for preview generation (e.g. Paint, Select, Extrude, Transform, Sculpt)
	 */
	bool previewNeedsExistingVolume(const Modifier &modifier) const;

	/**
	 * @brief Check if the brush can use a simple wireframe preview instead of a full volume
	 */
	bool isSimplePreview(const Brush *brush, const voxel::Region &region) const;

public:
	void construct();
	bool init(const ModifierRendererPtr &renderer);
	void shutdown();

	const BrushPreview &brushPreview() const;
	voxel::RawVolume *previewVolume() const;
	voxel::RawVolume *previewMirrorVolume() const;

	/**
	 * @brief Reset all preview state and free preview volumes
	 */
	void resetPreview();

	/**
	 * @brief Schedule a preview update for the next frame
	 */
	void scheduleUpdate(double nowSeconds);

	/**
	 * @brief Check and run pending preview updates
	 * @return true if a preview was generated
	 */
	bool checkPendingUpdate(double nowSeconds, Modifier &modifier, palette::Palette &activePalette,
							voxel::RawVolume *activeVolume, scenegraph::SceneGraph &sceneGraph);

	/**
	 * @brief Generate or update the brush preview volumes
	 */
	void updateBrushVolumePreview(Modifier &modifier, palette::Palette &activePalette,
								  voxel::RawVolume *activeVolume, scenegraph::SceneGraph &sceneGraph);
};

inline const BrushPreview &PreviewManager::brushPreview() const {
	return _brushPreview;
}

inline voxel::RawVolume *PreviewManager::previewVolume() const {
	return _previewVolume;
}

inline voxel::RawVolume *PreviewManager::previewMirrorVolume() const {
	return _previewMirrorVolume;
}

} // namespace voxedit
