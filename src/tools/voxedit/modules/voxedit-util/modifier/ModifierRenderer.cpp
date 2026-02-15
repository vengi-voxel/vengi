/**
 * @file
 */

#include "ModifierRenderer.h"
#include "../AxisUtil.h"
#include "color/Color.h"
#include "core/Log.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "video/Camera.h"
#include "video/ScopedState.h"
#include "video/ShapeBuilder.h"
#include "video/Types.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

namespace voxedit {

ModifierRenderer::ModifierRenderer() : _meshState(core::make_shared<voxel::MeshState>()) {
}

ModifierRenderer::ModifierRenderer(const voxel::MeshStatePtr &meshState) : _meshState(meshState) {
}

bool ModifierRenderer::init() {
	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}

	_meshState->construct();
	if (!_meshState->init()) {
		Log::error("Failed to initialize the mesh state");
		return false;
	}

	_volumeRenderer.construct();
	if (!_volumeRenderer.init(_meshState->hasNormals())) {
		Log::error("Failed to initialize the volume renderer");
		return false;
	}

	_shapeBuilder.clear();
	_shapeBuilder.setColor(color::alpha(color::SteelBlue(), 0.8f));
	_shapeBuilder.sphere(8, 6, 0.5f);
	_shapeRenderer.createOrUpdate(_referencePointMesh, _shapeBuilder);

	return true;
}

void ModifierRenderer::shutdown() {
	_mirrorMeshIndex = -1;
	_voxelCursorMesh = -1;
	_referencePointMesh = -1;
	for (int i = 0; i < lengthof(_aabbMeshes); ++i) {
		_aabbMeshes[i] = -1;
	}
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_volumeRendererCtx.shutdown();
	_volumeRenderer.shutdown();
	// the volumes in this state belong to the brush
	(void)_meshState->shutdown();
}

void ModifierRenderer::updateCursor(const voxel::Voxel& voxel, voxel::FaceNames face, bool flip) {
	_shapeBuilder.clear();
	video::ShapeBuilderCube flags = video::ShapeBuilderCube::All;
	switch (face) {
	case voxel::FaceNames::PositiveX:
		if (flip) {
			flags = video::ShapeBuilderCube::Left;
		} else {
			flags = video::ShapeBuilderCube::Right;
		}
		break;
	case voxel::FaceNames::PositiveY:
		if (flip) {
			flags = video::ShapeBuilderCube::Bottom;
		} else {
			flags = video::ShapeBuilderCube::Top;
		}
		break;
	case voxel::FaceNames::PositiveZ:
		if (flip) {
			flags = video::ShapeBuilderCube::Back;
		} else {
			flags = video::ShapeBuilderCube::Front;
		}
		break;
	case voxel::FaceNames::NegativeX:
		if (flip) {
			flags = video::ShapeBuilderCube::Right;
		} else {
			flags = video::ShapeBuilderCube::Left;
		}
		break;
	case voxel::FaceNames::NegativeY:
		if (flip) {
			flags = video::ShapeBuilderCube::Top;
		} else {
			flags = video::ShapeBuilderCube::Bottom;
		}
		break;
	case voxel::FaceNames::NegativeZ:
		if (flip) {
			flags = video::ShapeBuilderCube::Front;
		} else {
			flags = video::ShapeBuilderCube::Back;
		}
		break;
	case voxel::FaceNames::Max:
		return;
	}
	_shapeBuilder.setColor(color::alpha(color::Red(), 0.6f));
	_shapeBuilder.cube(glm::vec3(0.0f), glm::vec3(1.0f), flags);
	_shapeRenderer.createOrUpdate(_voxelCursorMesh, _shapeBuilder);
}

void ModifierRenderer::clear() {
	_volumeRenderer.clear(_meshState);
	for (int i = 0; i < lengthof(_aabbMeshes); ++i) {
		_shapeRenderer.deleteMesh(_aabbMeshes[i]);
		_aabbMeshes[i] = -1;
	}
}

void ModifierRenderer::updateBrushVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette) {
	// Note: We don't delete the returned old volume because ownership stays with the caller (ModifierFacade)
	// The caller manages the volume lifetime via ScopedPtr
	(void)_volumeRenderer.setVolume(_meshState, idx, volume, palette, nullptr, true);
	if (volume != nullptr) {
		_volumeRenderer.scheduleRegionExtraction(_meshState, idx, volume->region());
	}
}

void ModifierRenderer::updateBrushVolume(int idx, const voxel::Region &region, color::RGBA color) {
	core_assert(idx >= 0 && idx < lengthof(_aabbMeshes));
	_shapeBuilder.clear();
	_shapeBuilder.setColor(color::fromRGBA(color));
	_shapeBuilder.cube(region.getLowerCorner(), region.getUpperCorner() + glm::one<glm::ivec3>());
	_shapeRenderer.createOrUpdate(_aabbMeshes[idx], _shapeBuilder);
}

void ModifierRenderer::renderBrushVolume(const video::Camera &camera, const glm::mat4 &model) {
	if (_volumeRendererCtx.frameBuffer.dimension() != camera.size()) {
		_volumeRendererCtx.shutdown();
		_volumeRendererCtx.init(camera.size());
	}
	_meshState->extractAllPending();
	if (_meshState->volume(0) != nullptr) {
		_meshState->setModelMatrix(0, model, glm::vec3(0.0f), glm::vec3(0.0f));
	}
	if (_meshState->volume(1) != nullptr) {
		_meshState->setModelMatrix(1, model, glm::vec3(0.0f), glm::vec3(0.0f));
	}
	_volumeRenderer.update(_meshState);
	_volumeRenderer.render(_meshState, _volumeRendererCtx, camera, false, false);
}

void ModifierRenderer::updateMirrorPlane(math::Axis axis, const glm::ivec3 &mirrorPos, const voxel::Region &region) {
	if (axis == math::Axis::None) {
		if (_mirrorMeshIndex != -1) {
			_shapeRenderer.deleteMesh(_mirrorMeshIndex);
			_mirrorMeshIndex = -1;
		}
		return;
	}

	const glm::vec4 color = color::alpha(color::LightGray(), 0.3f);
	updateShapeBuilderForPlane(_shapeBuilder, region, true, mirrorPos, axis, color);
	_shapeRenderer.createOrUpdate(_mirrorMeshIndex, _shapeBuilder);
}

void ModifierRenderer::update(const ModifierRendererContext &ctx) {
	const bool flip = voxel::isAir(ctx.voxelAtCursor.getMaterial());
	updateCursor(ctx.cursorVoxel, ctx.cursorFace, flip);

	_cursorPosition = ctx.cursorPosition;
	_gridResolution = ctx.gridResolution;
	_referencePoint = glm::vec3(ctx.referencePosition) + 0.5f;

	if (ctx.mirrorAxis != _lastMirrorAxis || ctx.mirrorPos != _lastMirrorPos ||
		ctx.activeRegion != _lastActiveRegion) {
		updateMirrorPlane(ctx.mirrorAxis, ctx.mirrorPos, ctx.activeRegion);
		_lastMirrorAxis = ctx.mirrorAxis;
		_lastMirrorPos = ctx.mirrorPos;
		_lastActiveRegion = ctx.activeRegion;
	}

	// Update brush preview volumes
	if (ctx.brushActive) {
		if (ctx.useSimplePreview) {
			// Simple AABB preview using shape rendering
			updateBrushVolume(0, nullptr, nullptr);
			updateBrushVolume(1, nullptr, nullptr);
			if (ctx.simplePreviewRegion.isValid()) {
				updateBrushVolume(0, ctx.simplePreviewRegion, ctx.simplePreviewColor);
			}
			if (ctx.simpleMirrorPreviewRegion.isValid()) {
				updateBrushVolume(1, ctx.simpleMirrorPreviewRegion, ctx.simplePreviewColor);
			}
		} else {
			// Complex voxel-based preview
			updateBrushVolume(0, ctx.previewVolume, ctx.palette);
			updateBrushVolume(1, ctx.previewMirrorVolume, ctx.palette);
		}
	} else {
		clear();
	}
}

void ModifierRenderer::render(const video::Camera &camera, const glm::mat4 &modelMatrix) {
	video::ScopedState scopedDepth(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	{
		const video::ScopedState depthTest(video::State::DepthTest, false);
		const video::ScopedState cullFace(video::State::CullFace, false);
		_shapeRenderer.render(_referencePointMesh, camera, glm::translate(modelMatrix, _referencePoint));

		const glm::mat4 &translate = glm::translate(modelMatrix, glm::vec3(_cursorPosition));
		const glm::mat4 cursorMatrix = glm::scale(translate, glm::vec3((float)_gridResolution));
		_shapeRenderer.render(_voxelCursorMesh, camera, cursorMatrix);
	}

	for (int i = 0; i < lengthof(_aabbMeshes); ++i) {
		_shapeRenderer.render(_aabbMeshes[i], camera, modelMatrix);
	}

	// Render brush volume preview
	video::polygonOffset(glm::vec3(-0.1f));
	renderBrushVolume(camera, modelMatrix);
	video::polygonOffset(glm::vec3(0.0f));

	const video::ScopedState blend(video::State::Blend, true);
	_shapeRenderer.render(_mirrorMeshIndex, camera, modelMatrix);
}

void ModifierRenderer::waitForPendingExtractions() {
	_meshState->extractAllPending();
}

} // namespace voxedit
