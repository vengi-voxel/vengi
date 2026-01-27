/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/IComponent.h"
#include "core/SharedPtr.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace palette {
class Palette;
}

namespace voxel {
class RawVolume;
}

namespace voxedit {

/**
 * @brief Context passed to the modifier renderer for updating and rendering state.
 *
 * This aggregates all the information the renderer needs to update its internal buffers
 * and render the modifier visuals. The renderer implementation handles all buffer updates
 * internally based on this context.
 */
struct ModifierRendererContext {
	// Cursor info
	voxel::Voxel cursorVoxel;
	voxel::Voxel voxelAtCursor;
	voxel::FaceNames cursorFace = voxel::FaceNames::Max;
	glm::ivec3 cursorPosition{0};
	int gridResolution = 1;

	// Reference point
	glm::ivec3 referencePosition{0};

	// Mirror plane
	math::Axis mirrorAxis = math::Axis::None;
	glm::ivec3 mirrorPos{0};
	voxel::Region activeRegion;

	// Selections
	scenegraph::Selections selections;

	// Brush preview
	voxel::RawVolume *previewVolume = nullptr;
	voxel::RawVolume *previewMirrorVolume = nullptr;
	voxel::Region simplePreviewRegion;
	voxel::Region simpleMirrorPreviewRegion;
	color::RGBA simplePreviewColor{0};
	palette::Palette *palette = nullptr;
	bool useSimplePreview = false;
	bool brushActive = false;
};

class IModifierRenderer : public core::IComponent {
public:
	bool init() override {
		return true;
	}
	void shutdown() override {
	}

	/**
	 * @brief Update all internal renderer state based on the given context.
	 *
	 * The renderer implementation should handle:
	 * - Cursor visualization updates
	 * - Mirror plane updates
	 * - Selection buffer updates
	 * - Brush preview volume mesh updates
	 *
	 * @param ctx The context containing all necessary state for the update.
	 */
	virtual void update(const ModifierRendererContext &ctx) {
	}

	/**
	 * @brief Render all modifier visuals.
	 *
	 * @param camera The camera for rendering.
	 * @param modelMatrix The model transformation matrix.
	 */
	virtual void render(const video::Camera &camera, const glm::mat4 &modelMatrix) {
	}
};

using ModifierRendererPtr = core::SharedPtr<IModifierRenderer>;

} // namespace voxedit
