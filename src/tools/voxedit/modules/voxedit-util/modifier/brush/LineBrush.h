/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "LineState.h"
#include "voxel/Region.h"

namespace voxedit {

/**
 * @ingroup Brushes
 */
class LineBrush : public Brush {
private:
	using Super = Brush;

protected:
	LineState _state;
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	LineBrush() : Super(BrushType::Line) {
	}
	virtual ~LineBrush() = default;
	void update(const BrushContext &ctx, double nowSeconds) override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;
};

} // namespace voxedit
