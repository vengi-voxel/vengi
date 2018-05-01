/**
 * @file
 */

#pragma once

#include "SelectType.h"
#include "core/GLM.h"

namespace voxel {
class RawVolume;
}

namespace voxedit {

class SelectionHandler {
private:
	int _selectedVoxels = 0;
	SelectType _selectionType = SelectType::Single;
public:
	void setSelectionType(SelectType type);
	SelectType selectionType() const;

	bool select(const voxel::RawVolume* volume, voxel::RawVolume* selectionVolume, const glm::ivec3& pos);
	void unselectAll();
	int selectedVoxels() const;
};

inline void SelectionHandler::setSelectionType(SelectType type) {
	_selectionType = type;
}

inline SelectType SelectionHandler::selectionType() const {
	return _selectionType;
}

inline int SelectionHandler::selectedVoxels() const {
	return _selectedVoxels;
}

}
