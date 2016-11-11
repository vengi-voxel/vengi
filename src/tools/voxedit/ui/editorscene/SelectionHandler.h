#pragma once

#include "SelectType.h"
#include "core/GLM.h"

namespace voxel {
class RawVolume;
}

namespace voxedit {

class SelectionHandler {
private:
	SelectType _selectionType = SelectType::Single;
public:
	void setSelectionType(SelectType type);
	SelectType selectionType() const;

	bool select(const voxel::RawVolume* volume, voxel::RawVolume* selectionVolume, const glm::ivec3& pos);
};

inline void SelectionHandler::setSelectionType(SelectType type) {
	_selectionType = type;
}

inline SelectType SelectionHandler::selectionType() const {
	return _selectionType;
}

}
