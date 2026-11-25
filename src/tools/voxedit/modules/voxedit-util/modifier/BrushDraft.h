/**
 * @file
 */

#pragma once

#include "BrushApply.h"
#include "core/Function.h"
#include "core/UUID.h"
#include "core/collection/Buffer.h"
#include "voxel/Region.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

/** Adjustable-operation lifetime owned by the host, independent of concrete tools. */
class BrushDraft {
public:
	virtual ~BrushDraft() = default;
	virtual bool active() const = 0;
	virtual bool enabled() const {
		return true;
	}
	virtual bool requiresSelection() const = 0;
	virtual bool valid(const scenegraph::SceneGraph &scene) const = 0;
	virtual BrushApplyResult refresh(scenegraph::SceneGraph &scene, const core::Buffer<core::UUID> &targets,
									 const voxel::Region &constraint, const BrushContext &input,
									 const BrushApplyOptions &options,
									 const core::Function<BrushApplyResult()> &finishPrevious) = 0;
	virtual BrushApplyResult apply(scenegraph::SceneGraph &scene) = 0;
	virtual void clear() = 0;
	virtual const scenegraph::SceneGraph *view() const = 0;
	virtual bool paletteChanged(const core::UUID &uuid) const = 0;
	virtual bool normalPaletteChanged(const core::UUID &uuid) const = 0;
	virtual voxel::Region dirtyRegion(const core::UUID &uuid) const = 0;
};

} // namespace voxedit
