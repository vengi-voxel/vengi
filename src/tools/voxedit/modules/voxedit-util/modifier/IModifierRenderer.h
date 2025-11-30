/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "color/RGBA.h"
#include "core/SharedPtr.h"
#include "math/Axis.h"
#include "video/Camera.h"
#include "voxedit-util/modifier/Selection.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"

namespace palette {
class Palette;
}

namespace voxel {
class RawVolume;
}

namespace voxedit {

class IModifierRenderer : public core::IComponent {
public:
	bool init() override {
		return true;
	}
	void shutdown() override {
	}

	virtual void renderBrushVolume(const video::Camera &camera, const glm::mat4 &model) {
	}
	virtual void clear() {
	}
	virtual void updateBrushVolume(int idx, const voxel::Region &region, color::RGBA color) {
	}
	virtual void updateBrushVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette) {
	}

	virtual void render(const video::Camera &camera, const glm::mat4 &cursor, const glm::mat4 &model) {
	}
	virtual void renderSelection(const video::Camera &camera, const glm::mat4 &model) {
	}

	virtual void updateReferencePosition(const glm::ivec3 &pos) {
	}
	virtual void updateMirrorPlane(math::Axis axis, const glm::ivec3 &mirrorPos, const voxel::Region &region) {
	}
	virtual void updateSelectionBuffers(const Selections &selections) {
	}
	virtual void updateCursor(const voxel::Voxel &voxel, voxel::FaceNames face, bool flip) {
	}
};

using ModifierRendererPtr = core::SharedPtr<IModifierRenderer>;

} // namespace voxedit
