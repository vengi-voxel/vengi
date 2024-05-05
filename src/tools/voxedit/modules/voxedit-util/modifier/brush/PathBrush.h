/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "LineState.h"
#include "voxelutil/Connectivity.h"

namespace voxedit {

class PathBrush : public Brush {
private:
	using Super = Brush;
	LineState _state;
	voxelutil::Connectivity _connectivity = voxelutil::Connectivity::EighteenConnected;

public:
	PathBrush() : Super(BrushType::Path) {
	}
	virtual ~PathBrush() = default;
	void update(const BrushContext &ctx, double nowSeconds) override;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;

	void setConnectivity(voxelutil::Connectivity connectivity);
	voxelutil::Connectivity connectivity() const;
};

inline void PathBrush::setConnectivity(voxelutil::Connectivity connectivity) {
	_connectivity = connectivity;
	markDirty();
}

inline voxelutil::Connectivity PathBrush::connectivity() const {
	return _connectivity;
}

} // namespace voxedit
