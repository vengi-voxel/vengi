/**
 * @file
 */

#pragma once

#include "ModifierType.h"
#include "brush/Brush.h"
#include "math/Axis.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
}

namespace voxel {
class RawVolume;
}

namespace voxedit {

/**
 * @brief Outcome of applying one sample to one target.
 *
 * Status and the dirty region are independent: a Failed apply may still have
 * written voxels, and those writes must remain visible to the caller.
 */
enum class BrushApplyStatus { Unchanged, Changed, InvalidTarget, Failed };

/**
 * @brief One evaluated dab. Const input for session and preview evaluation.
 *
 * Settings and typed runtime belong to the supplied evaluator instance,
 * which is private to a target or preview.
 */
struct BrushSample {
	glm::ivec3 position{0};
	voxel::FaceNames face = voxel::FaceNames::Max;
	ModifierType operation = ModifierType::Place;
	math::Axis lockedAxis = math::Axis::None;
	glm::ivec3 gridResolution{1};
	voxel::Voxel cursorVoxel;
	voxel::Voxel hitCursorVoxel;
	voxel::Voxel voxelAtCursor;
	bool fixedOrthoSideView = false;
	bool normalPaint = false;
	uint8_t normalIndex = NO_NORMAL;
};

struct BrushApplyResult {
	BrushApplyStatus status = BrushApplyStatus::Unchanged;
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;
	bool paletteChanged = false;
	bool normalPaletteChanged = false;

	bool changed() const {
		return dirtyRegion.isValid() || paletteChanged || normalPaletteChanged;
	}
	void accumulate(const BrushApplyResult &other) {
		paletteChanged |= other.paletteChanged;
		normalPaletteChanged |= other.normalPaletteChanged;
		if (other.dirtyRegion.isValid()) {
			if (dirtyRegion.isValid()) {
				dirtyRegion.accumulate(other.dirtyRegion);
			} else {
				dirtyRegion = other.dirtyRegion;
			}
		}
		if (other.failed()) {
			status = other.status;
		} else if (!failed()) {
			status = changed() ? BrushApplyStatus::Changed : BrushApplyStatus::Unchanged;
		}
	}
	bool failed() const {
		return status == BrushApplyStatus::Failed || status == BrushApplyStatus::InvalidTarget;
	}
};

/**
 * @brief Writable target for one apply.
 *
 * @c volume is the identity captured at edit begin. @c writeVolume is where
 * voxels are written (model or slice scratch). Preview callers pass a scratch
 * node, scratch volume, and private evaluator runtime.
 */
struct BrushApplyTarget {
	scenegraph::SceneGraphNode *node = nullptr;
	voxel::RawVolume *volume = nullptr;
	voxel::RawVolume *writeVolume = nullptr;
	voxel::Region bounds = voxel::Region::InvalidRegion;
	voxel::Region selectionRegion = voxel::Region::InvalidRegion;
	voxel::Region sliceRegion = voxel::Region::InvalidRegion;
};

struct BrushApplyOptions {
	bool preview = false;
	bool applySelectionPolicy = false;
	bool autoSelect = false;
};

/**
 * @brief Clamp a brush region that would leave the volume, matching the legacy cursor shift.
 */
glm::ivec3 clampBrushCursor(const voxel::Region &volumeRegion, const voxel::Region &brushRegion,
							const glm::ivec3 &cursor);

/**
 * @brief Evaluate one sample on one target without mutating @p settings.
 *
 * Does not notify the scene or record history. Undo policy belongs to the caller.
 */
BrushApplyResult applyBrushSample(Brush &brush, scenegraph::SceneGraph &sceneGraph, const BrushSample &sample,
								  BrushApplyTarget &target, const BrushContext &settings,
								  const BrushApplyOptions &options = {});

} // namespace voxedit
