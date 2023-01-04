/**
 * @file
 */

#include "ModifierRenderer.h"
#include "core/Color.h"
#include "math/Axis.h"
#include "video/Camera.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "video/ShapeBuilder.h"
#include "video/Types.h"
#include "voxedit-util/SceneManager.h"
#include "../AxisUtil.h"
#include "voxedit-util/modifier/Selection.h"

namespace voxedit {

bool ModifierRenderer::init() {
	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}

	return true;
}

void ModifierRenderer::shutdown() {
	_mirrorMeshIndex = -1;
	_aabbMeshIndex = -1;
	_selectionIndex = -1;
	_voxelCursorMesh = -1;
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
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
	_shapeBuilder.setColor(core::Color::alpha(core::Color::Red, 0.6f));
	_shapeBuilder.cube(glm::vec3(0.0f), glm::vec3(1.0f), flags);
	_shapeRenderer.createOrUpdate(_voxelCursorMesh, _shapeBuilder);
}

void ModifierRenderer::updateSelectionBuffers(const Selection& selection) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Yellow);
	_shapeBuilder.aabb(selection.getLowerCorner(), selection.getUpperCorner() + glm::one<glm::ivec3>());
	_shapeRenderer.createOrUpdate(_selectionIndex, _shapeBuilder);
}

void ModifierRenderer::updateAABBMirrorMesh(const glm::vec3& mins, const glm::vec3& maxs,
		const glm::vec3& minsMirror, const glm::vec3& maxsMirror) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::Red, 0.5f));
	_shapeBuilder.cube(mins, maxs);
	_shapeBuilder.cube(minsMirror, maxsMirror);
	_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
}

void ModifierRenderer::updateAABBMesh(const glm::vec3& mins, const glm::vec3& maxs) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::Red, 0.5f));
	_shapeBuilder.cube(mins, maxs);
	_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
}

void ModifierRenderer::renderAABBMode(const video::Camera& camera) {
	static const glm::vec2 offset(-0.25f, -0.5f);
	video::ScopedPolygonMode polygonMode(camera.polygonMode(), offset);
	_shapeRenderer.render(_aabbMeshIndex, camera);
}

void ModifierRenderer::render(const video::Camera& camera, const glm::mat4& model) {
	const video::ScopedState depthTest(video::State::DepthTest, false);
	const video::ScopedState cullFace(video::State::CullFace, false);
	_shapeRenderer.render(_voxelCursorMesh, camera, model);
	_shapeRenderer.render(_mirrorMeshIndex, camera);
}

void ModifierRenderer::renderSelection(const video::Camera& camera) {
	_shapeRenderer.render(_selectionIndex, camera);
}

void ModifierRenderer::updateMirrorPlane(math::Axis axis, const glm::ivec3& mirrorPos) {
	if (axis == math::Axis::None) {
		if (_mirrorMeshIndex != -1) {
			_shapeRenderer.deleteMesh(_mirrorMeshIndex);
			_mirrorMeshIndex = -1;
		}
		return;
	}

	updateShapeBuilderForPlane(_shapeBuilder, sceneMgr().sceneGraph().region(), true, mirrorPos, axis,
			core::Color::alpha(core::Color::LightGray, 0.3f));
	_shapeRenderer.createOrUpdate(_mirrorMeshIndex, _shapeBuilder);
}

}
