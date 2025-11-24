/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/Connectivity.h"

namespace voxedit {

/**
 * @brief Pathfinding brush that walks on existing volumes from reference position to cursor position
 * @ingroup Brushes
 */
class PathBrush : public Brush {
private:
	using Super = Brush;

protected:
	voxel::Connectivity _connectivity = voxel::Connectivity::EighteenConnected;
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;

public:
	PathBrush() : Super(BrushType::Path, ModifierType::Place, ModifierType::Place) {
	}
	virtual ~PathBrush() = default;

	void setConnectivity(voxel::Connectivity connectivity);
	voxel::Connectivity connectivity() const;
};

inline void PathBrush::setConnectivity(voxel::Connectivity connectivity) {
	_connectivity = connectivity;
}

inline voxel::Connectivity PathBrush::connectivity() const {
	return _connectivity;
}

} // namespace voxedit
