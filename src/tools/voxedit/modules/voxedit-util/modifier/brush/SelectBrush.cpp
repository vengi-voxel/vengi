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

voxel::Region SelectBrush::calcSelectionRegion(const glm::ivec3 &cursorPosition) const {
	const glm::ivec3 &mins = glm::min(_selectStartPosition, cursorPosition);
	const glm::ivec3 &maxs = glm::max(_selectStartPosition, cursorPosition);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
