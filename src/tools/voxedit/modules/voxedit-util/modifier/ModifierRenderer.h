/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "math/Axis.h"
#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "voxedit-util/modifier/Selection.h"
#include "voxel/Voxel.h"

namespace voxedit {

class ModifierRenderer : public core::IComponent {
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int32_t _aabbMeshIndex = -1;
	int32_t _selectionIndex = -1;
	int32_t _mirrorMeshIndex = -1;
	int32_t _voxelCursorMesh = -1;

public:
	bool init() override;
	void shutdown() override;

	void render(const video::Camera& camera, const glm::mat4& model);
	void renderAABBMode(const video::Camera& camera);
	void renderSelection(const video::Camera& camera);

	void updateAABBMesh(const glm::vec3& mins, const glm::vec3& maxs);
	void updateAABBMirrorMesh(const glm::vec3 &mins, const glm::vec3 &maxs, const glm::vec3 &minsMirror,
							  const glm::vec3 &maxsMirror);
	void updateMirrorPlane(math::Axis axis, const glm::ivec3 &mirrorPos);
	void updateSelectionBuffers(const Selection &selection);
	void updateCursor(const voxel::Voxel &voxel);
};

}
