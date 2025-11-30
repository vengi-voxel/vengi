/**
 * @file
 */

#pragma once

#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxelrender/RawVolumeRenderer.h"

namespace voxedit {

class ModifierRenderer : public IModifierRenderer {
private:
	voxel::MeshStatePtr _meshState;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	voxelrender::RawVolumeRenderer _volumeRenderer;
	voxelrender::RenderContext _volumeRendererCtx;
	int32_t _selectionIndex = -1;
	int32_t _mirrorMeshIndex = -1;
	int32_t _voxelCursorMesh = -1; // TODO: remove me - should be a brush - see issue #130
	int32_t _referencePointMesh = -1;
	glm::vec3 _referencePoint{0.0f};
	int32_t _aabbMeshes[2] { -1, -1 };

public:
	ModifierRenderer();
	ModifierRenderer(const voxel::MeshStatePtr &meshState);
	bool init() override;
	void shutdown() override;

	void render(const video::Camera &camera, const glm::mat4 &cursor, const glm::mat4 &model) override;
	void renderBrushVolume(const video::Camera &camera, const glm::mat4 &model) override;
	void renderSelection(const video::Camera &camera, const glm::mat4 &model) override;
	void clear() override;
	/**
	 * @note The given volume must still get freed by the caller - the renderer is not taking overship.
	 *
	 * But it is keeping a pointer to the volume!
	 */
	void updateBrushVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette) override;
	void updateBrushVolume(int idx, const voxel::Region &region, color::RGBA color) override;
	void updateReferencePosition(const glm::ivec3 &pos) override;
	void updateMirrorPlane(math::Axis axis, const glm::ivec3 &mirrorPos, const voxel::Region &sceneRegion) override;
	void updateSelectionBuffers(const Selections &selections) override;
	void updateCursor(const voxel::Voxel &voxel, voxel::FaceNames face, bool flip) override;
};

} // namespace voxedit
