/**
 * @file
 */

#pragma once

#include "voxedit-util/modifier/IModifierRenderer.h"
#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "voxelrender/RawVolumeRenderer.h"

namespace voxedit {

class ModifierRenderer : public IModifierRenderer {
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	voxelrender::RawVolumeRenderer _volumeRenderer;
	voxelrender::RenderContext _volumeRendererCtx;
	int32_t _selectionIndex = -1;
	int32_t _mirrorMeshIndex = -1;
	int32_t _voxelCursorMesh = -1;
	int32_t _referencePointMesh = -1;
	glm::mat4 _referencePointModelMatrix{1.0f};

public:
	bool init() override;
	void shutdown() override;

	void render(const video::Camera& camera, const glm::mat4& model) override;
	void renderShape(const video::Camera &camera) override;
	void renderSelection(const video::Camera& camera) override;
	void clearShapeMeshes() override;
	void updateShapeMesh(int idx, voxel::RawVolume *volume, voxel::Palette *palette) override;
	void updateReferencePosition(const glm::ivec3 &pos) override;
	void updateMirrorPlane(math::Axis axis, const glm::ivec3 &mirrorPos) override;
	void updateSelectionBuffers(const Selections &selections) override;
	void updateCursor(const voxel::Voxel &voxel, voxel::FaceNames face, bool flip) override;
};

}
