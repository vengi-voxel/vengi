/**
 * @file
 */

#include "SelectBrush.h"

namespace voxedit {

bool SelectBrush::active() const {
	return _selectStartPositionValid;
}

void SelectBrush::start(const glm::ivec3 &startPos) {
	_selectStartPosition = startPos;
	_selectStartPositionValid = true;
}

void SelectBrush::stop() {
	_selectStartPositionValid = false;
}

void SelectBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
}

voxel::Region SelectBrush::calcRegion(const BrushContext &context) const {
	const glm::ivec3 &mins = glm::min(_selectStartPosition, context.cursorPosition);
	const glm::ivec3 &maxs = glm::max(_selectStartPosition, context.cursorPosition);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
