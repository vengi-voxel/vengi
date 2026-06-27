/**
 * @file
 */

#include "Line.h"
#include "Circle.h"
#include "math/Axis.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"
#include "voxel/Face.h"
#include "voxelutil/VolumeSelect.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace voxedit {
namespace select {

void Line::reset() {
	Super::reset();
	_previewActive = false;
}

void Line::abort(BrushContext &ctx) {
	Super::abort(ctx);
	_previewActive = false;
}

void Line::endBrush(BrushContext &ctx) {
	Super::endBrush(ctx);
	_previewActive = false;
}

bool Line::wantBrushGizmo(const BrushContext &ctx) const {
	return _previewActive;
}

void Line::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	const glm::mat4 model = _sceneManager != nullptr ? _sceneManager->worldMatrix() : glm::mat4(1.0f);
	auto toWorld = [&model](const glm::ivec3 &local) {
		// Center on the voxel so the line runs through voxel centers.
		return glm::vec3(model * glm::vec4(glm::vec3(local) + 0.5f, 1.0f));
	};
	state.operations = BrushGizmo_Line;
	state.positions[0] = toWorld(_previewStart);
	state.positions[1] = toWorld(_previewEnd);
	state.numPositions = 2;
}

void Line::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					const voxel::Region &region, const AABBBrushState &state) {
	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	// The two drag endpoints define the segment.
	if (_pathMode == PathMode::FollowSurface && state.aabbFace != voxel::FaceNames::Max) {
		int uAxis;
		int vAxis;
		Circle::ellipseAxes(state.aabbFace, uAxis, vAxis);
		const int wAxis = math::getIndexForAxis(voxel::faceToAxis(state.aabbFace));
		const bool positiveNormal = voxel::isPositiveFace(state.aabbFace);
		// Drape the line over the visible surface between the endpoints, filling the riser faces,
		// so it stays continuous AND visible across terraces (no per-column gaps, no hidden risers).
		voxelutil::lineDrapeSurface(wrapper, state.aabbFirstPos, state.cursorPosition, uAxis, vAxis, wAxis,
									positiveNormal, _width, ctx.targetVolumeRegion, func);
	} else {
		voxelutil::lineMarkSolid(wrapper, state.aabbFirstPos, state.cursorPosition, _width, ctx.targetVolumeRegion, func);
	}
}

} // namespace select
} // namespace voxedit
