/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/TimedValue.h"
#include "core/collection/Buffer.h"
#include "render/GridRenderer.h"
#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "math/Mat4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxedit {

class ModifierRenderer : public IModifierRenderer {
private:
	voxel::MeshStatePtr _meshState;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	render::GridRenderer _gridRenderer;
	voxelrender::RawVolumeRenderer _volumeRenderer;
	core::VarPtr _showGrid;
	core::VarPtr _showLockedAxis;
	core::VarPtr _showAABB;
	core::VarPtr _planeSize;
	core::VarPtr _showPlane;
	int32_t _mirrorMeshIndex = -1;
	int32_t _highlightMesh = -1;
	int32_t _voxelCursorMesh = -1; // TODO: remove me - should be a brush - see issue #130
	int32_t _referencePointMesh = -1;
	math::Vec3f _referencePoint{0.0f};
	int32_t _aabbMeshes[2]{-1, -1};
	int32_t _lockedAxisIndices[3] = {-1, -1, -1};

	// State tracking to avoid redundant updates
	math::Axis _lastMirrorAxis = math::Axis::None;
	math::Axis _lastLockedAxis = math::Axis::None;
	math::Vec3i _lastMirrorPos{0};
	voxel::Region _lastMirrorRegion;
	voxel::Region _lastLockedAxisRegion;

	// Cursor state for rendering
	math::Vec3i _cursorPosition{0};
	math::Vec3i _gridResolution{1};
	voxel::Region _activeRegion;
	bool _previewErase = false;
	using TimedRegion = core::TimedValue<voxel::Region>;
	TimedRegion _highlightRegion;

	void updateCursor(const voxel::Voxel &voxel, voxel::FaceNames face, bool flip);
	void updateMirrorPlane(math::Axis axis, const math::Vec3i &mirrorPos, const voxel::Region &sceneRegion);
	void updateLockedPlane(math::Axis lockedAxis, math::Axis axis, const math::Vec3i &cursorPosition, const voxel::Region &region);
	void updateBrushVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette, palette::NormalPalette *normalPalette = nullptr);
	void updateBrushVolume(int idx, const voxel::Region &region, color::RGBA color);
	void updateBrushOutline(int idx, const voxel::Region &region, color::RGBA color);
	void updateBrushPolylineOutline(int idx, const core::Buffer<math::Vec3f> &points, color::RGBA color);
	void clear();
	void handleCommandBuffer();

	void renderBrushVolume(voxelrender::RenderContext &renderContext, const video::Camera &camera, const math::Mat4f &model);

public:
	ModifierRenderer(const core::TimeProviderPtr &timeProvider);
	ModifierRenderer(const core::TimeProviderPtr &timeProvider, const voxel::MeshStatePtr &meshState);
	bool init() override;
	void shutdown() override;

	void clearBrushVolumes() override;
	void update(const ModifierRendererContext &ctx) override;
	void render(voxelrender::RenderContext &renderContext, const video::Camera &camera, const math::Mat4f &modelMatrix) override;
	void waitForPendingExtractions() override;
};

} // namespace voxedit
