/**
 * @file
 */

#pragma once

#include "render/ShapeRenderer.h"
#include "scenegraph/SceneGraphNode.h"
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
	int32_t _aabbMeshes[2]{-1, -1};

	// State tracking to avoid redundant updates
	math::Axis _lastMirrorAxis = math::Axis::None;
	glm::ivec3 _lastMirrorPos{0};
	voxel::Region _lastActiveRegion;

	// Cursor state for rendering
	glm::ivec3 _cursorPosition{0};
	int _gridResolution = 1;

	void updateCursor(const voxel::Voxel &voxel, voxel::FaceNames face, bool flip);
	void updateSelectionBuffers(const scenegraph::Selections &selections);
	void updateMirrorPlane(math::Axis axis, const glm::ivec3 &mirrorPos, const voxel::Region &sceneRegion);
	void updateBrushVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette);
	void updateBrushVolume(int idx, const voxel::Region &region, color::RGBA color);
	void clear();

	void renderBrushVolume(const video::Camera &camera, const glm::mat4 &model);

public:
	ModifierRenderer();
	ModifierRenderer(const voxel::MeshStatePtr &meshState);
	bool init() override;
	void shutdown() override;

	void update(const ModifierRendererContext &ctx) override;
	void render(const video::Camera &camera, const glm::mat4 &modelMatrix) override;
};

} // namespace voxedit
