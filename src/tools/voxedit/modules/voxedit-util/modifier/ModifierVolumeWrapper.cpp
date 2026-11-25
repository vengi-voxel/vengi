/**
 * @file
 */

#include "ModifierVolumeWrapper.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

ModifierVolumeWrapper::ModifierVolumeWrapper(scenegraph::SceneGraphNode &node, ModifierType modifierType,
											 const voxel::Region &box3DSelectionRegion)
	: Super(node.volume()), _modifierType(modifierType), _node(node) {
	_erase = (_modifierType & ModifierType::Erase) != ModifierType::None;
	_override = _modifierType == ModifierType::Override;
	_paint = _modifierType == ModifierType::Paint;
	_normalPaint = (_modifierType & ModifierType::NormalPaint) != ModifierType::None;
	_hasSelection = _node.hasSelection();
	_box3DSelectionRegion = box3DSelectionRegion;
}

void ModifierVolumeWrapper::growSelectionToNewVoxels() {
	if (!_hasSelection || !_dirtyRegion.isValid()) {
		return;
	}
	auto func = [this](int x, int y, int z, const voxel::Voxel & /*solidVoxel*/) {
		setFlagAt(x, y, z, voxel::FlagOutline);
	};
	voxelutil::visitVolumeParallel(*this, _dirtyRegion, func, voxelutil::VisitSolid());
}

void ModifierVolumeWrapper::autoSelectNewVoxels() {
	if (!_dirtyRegion.isValid()) {
		return;
	}
	if (_hasSelection) {
		_node.clearSelection();
	}
	auto func = [this](int x, int y, int z, const voxel::Voxel & /*solidVoxel*/) {
		setFlagAt(x, y, z, voxel::FlagOutline);
	};
	voxelutil::visitVolumeParallel(*this, _dirtyRegion, func, voxelutil::VisitSolid());
}

} // namespace voxedit
