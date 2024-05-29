/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "LineState.h"
#include "voxelutil/Connectivity.h"

namespace voxedit {

/**
 * @brief Pathfinding brush that walks on existing volumes from reference position to cursor position
 * @ingroup Brushes
 */
class PathBrush : public Brush {
private:
	using Super = Brush;

protected:
	LineState _state;
	voxelutil::Connectivity _connectivity = voxelutil::Connectivity::EighteenConnected;
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &context,
				  const voxel::Region &region) override;
	voxel::Region calcRegion(const BrushContext &context) const override;

public:
	PathBrush() : Super(BrushType::Path) {
	}
	virtual ~PathBrush() = default;
	void update(const BrushContext &ctx, double nowSeconds) override;

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
