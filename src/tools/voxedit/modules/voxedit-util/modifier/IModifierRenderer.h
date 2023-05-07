/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/SharedPtr.h"
#include "math/Axis.h"
#include "video/Camera.h"
#include "voxedit-util/modifier/Selection.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"

namespace voxedit {

class IModifierRenderer : public core::IComponent {
public:
	bool init() override {
		return true;
	}
	void shutdown() override {
	}

	virtual void render(const video::Camera &camera, const glm::mat4 &model) {
	}
	virtual void renderAABBMode(const video::Camera &camera) {
	}
	virtual void renderSelection(const video::Camera &camera) {
	}

	virtual void updateReferencePosition(const glm::ivec3 &pos) {
	}
	virtual void updateAABBMesh(const glm::vec3 &mins, const glm::vec3 &maxs) {
	}
	virtual void updateAABBMirrorMesh(const glm::vec3 &mins, const glm::vec3 &maxs, const glm::vec3 &minsMirror,
									  const glm::vec3 &maxsMirror) {
	}
	virtual void updateMirrorPlane(math::Axis axis, const glm::ivec3 &mirrorPos) {
	}
	virtual void updateSelectionBuffers(const Selections &selections) {
	}
	virtual void updateCursor(const voxel::Voxel &voxel, voxel::FaceNames face, bool flip) {
	}
};

using ModifierRendererPtr = core::SharedPtr<IModifierRenderer>;

} // namespace voxedit
