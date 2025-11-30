/**
 * @file
 */

#include "ModifierRenderer.h"
#include "color/Color.h"
#include "math/Axis.h"
#include "core/Log.h"
#include "video/Camera.h"
#include "video/ScopedState.h"
#include "video/ShapeBuilder.h"
#include "video/Types.h"
#include "../AxisUtil.h"
#include "voxedit-util/modifier/Selection.h"
#include "palette/Palette.h"
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
	_shapeBuilder.setColor(core::Color::alpha(core::Color::SteelBlue(), 0.8f));
	_shapeBuilder.sphere(8, 6, 0.5f);
	_shapeRenderer.createOrUpdate(_referencePointMesh, _shapeBuilder);

	return true;
}

void ModifierRenderer::shutdown() {
	_mirrorMeshIndex = -1;
	_selectionIndex = -1;
	_voxelCursorMesh = -1;
	_referencePointMesh = -1;
	for (int i = 0; i < lengthof(_aabbMeshes); ++i) {
		_aabbMeshes[i] = -1;
	}
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_volumeRendererCtx.shutdown();
	// this is automatically deleted in the ModifierFacade
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
	_shapeBuilder.setColor(core::Color::alpha(core::Color::Red(), 0.6f));
	_shapeBuilder.cube(glm::vec3(0.0f), glm::vec3(1.0f), flags);
	_shapeRenderer.createOrUpdate(_voxelCursorMesh, _shapeBuilder);
}

void ModifierRenderer::updateSelectionBuffers(const Selections& selections) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Yellow());
	for (const Selection &selection : selections) {
		if (!selection.isValid()) {
			continue;
		}
		_shapeBuilder.aabb(selection.getLowerCorner(), selection.getUpperCorner() + glm::one<glm::ivec3>());
	}
	_shapeRenderer.createOrUpdate(_selectionIndex, _shapeBuilder);
}

void ModifierRenderer::clear() {
	_volumeRenderer.clear(_meshState);
	for (int i = 0; i < lengthof(_aabbMeshes); ++i) {
		_shapeRenderer.deleteMesh(_aabbMeshes[i]);
		_aabbMeshes[i] = -1;
	}
}

void ModifierRenderer::updateBrushVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette) {
	delete _volumeRenderer.setVolume(_meshState, idx, volume, palette, nullptr, true);
	if (volume != nullptr) {
		_volumeRenderer.scheduleRegionExtraction(_meshState, idx, volume->region());
	}
}

void ModifierRenderer::updateBrushVolume(int idx, const voxel::Region &region, core::RGBA color) {
	core_assert(idx >= 0 && idx < lengthof(_aabbMeshes));
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::fromRGBA(color));
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
	_volumeRenderer.render(_meshState, _volumeRendererCtx, camera, false/*, model*/);
}

void ModifierRenderer::render(const video::Camera& camera, const glm::mat4 &cursor, const glm::mat4& model) {
	const video::ScopedState depthTest(video::State::DepthTest, false);
	const video::ScopedState cullFace(video::State::CullFace, false);
	_shapeRenderer.render(_voxelCursorMesh, camera, cursor);
	_shapeRenderer.render(_mirrorMeshIndex, camera, model);
	_shapeRenderer.render(_referencePointMesh, camera, glm::translate(model, _referencePoint));

	video::ScopedState scopedDepth(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);

	for (int i = 0; i < lengthof(_aabbMeshes); ++i) {
		_shapeRenderer.render(_aabbMeshes[i], camera, model);
	}
}

void ModifierRenderer::updateReferencePosition(const glm::ivec3 &pos) {
	const glm::vec3 posAligned((float)pos.x + 0.5f, (float)pos.y + 0.5f, (float)pos.z + 0.5f);
	_referencePoint = posAligned;
}

void ModifierRenderer::renderSelection(const video::Camera& camera, const glm::mat4 &model) {
	_shapeRenderer.render(_selectionIndex, camera, model);
}

void ModifierRenderer::updateMirrorPlane(math::Axis axis, const glm::ivec3& mirrorPos, const voxel::Region &region) {
	if (axis == math::Axis::None) {
		if (_mirrorMeshIndex != -1) {
			_shapeRenderer.deleteMesh(_mirrorMeshIndex);
			_mirrorMeshIndex = -1;
		}
		return;
	}

	const glm::vec4 color = core::Color::alpha(core::Color::LightGray(), 0.3f);
	updateShapeBuilderForPlane(_shapeBuilder, region, true, mirrorPos, axis, color);
	_shapeRenderer.createOrUpdate(_mirrorMeshIndex, _shapeBuilder);
}

}
