/**
 * @file
 */

#include "SelectionManager.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"

namespace voxedit {

const Selections &SelectionManager::selections() const {
	return _selections;
}

void SelectionManager::setMaxRegionSize(const voxel::Region &maxRegion) {
	_maxRegion = maxRegion;
}

void SelectionManager::invert(scenegraph::SceneGraphNode &node) {
	if (!node.isModelNode()) {
		return;
	}
	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		return;
	}
	if (!hasSelection()) {
		select(node, volume->region().getLowerCorner(), volume->region().getUpperCorner());
	} else {
		const Selections &remainingSelections = voxel::Region::subtract(volume->region(), _selections);
		reset();
		for (const Selection &selection : remainingSelections) {
			select(node, selection.getLowerCorner(), selection.getUpperCorner());
		}
	}
}

void SelectionManager::unselect(scenegraph::SceneGraphNode &node) {
	if (!node.isModelNode()) {
		return;
	}
	reset();
}

void SelectionManager::reset() {
	_selections.clear();
	markDirty();
}

const voxel::Region &SelectionManager::region() {
	if (!dirty()) {
		return _cachedRegion;
	}
	if (_selections.empty()) {
		return voxel::Region::InvalidRegion;
	}
	voxel::Region r = _selections[0];
	for (const Selection &selection : _selections) {
		r.accumulate(selection);
	}
	_cachedRegion = r;
	markClean();
	return _cachedRegion;
}

bool SelectionManager::select(scenegraph::SceneGraphNode &node, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
	if (!node.isModelNode()) {
		return false;
	}
	const Selection sel{mins, maxs};
	if (!sel.isValid()) {
		return false;
	}
	for (size_t i = 0; i < _selections.size(); ++i) {
		const Selection &s = _selections[i];
		if (s.containsRegion(sel)) {
			return true;
		}
	}

	for (size_t i = 0; i < _selections.size();) {
		Selection &s = _selections[i];
		if (sel.containsRegion(s)) {
			_selections.erase(i);
		} else if (voxel::intersects(sel, s)) {
			const Selections newRegions = voxel::Region::subtract(s, sel);
			_selections.erase(i);
			_selections.insert(_selections.begin() + i, newRegions.begin(), newRegions.end());
			i += newRegions.size();
		} else {
			++i;
		}
	}
	_selections.push_back(sel);
	markDirty();
	return true;
}

void SelectionManager::selectAll(scenegraph::SceneGraphNode &node) {
	if (!node.isModelNode()) {
		return;
	}
	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		return;
	}
	select(node, volume->region().getLowerCorner(), volume->region().getUpperCorner());
}

bool SelectionManager::unselect(scenegraph::SceneGraphNode &node, const glm::ivec3 &pos) {
	if (!node.isModelNode()) {
		return false;
	}
	return unselect(node, pos, pos);
}

bool SelectionManager::unselect(scenegraph::SceneGraphNode &node, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
	if (!node.isModelNode()) {
		return false;
	}
	const Selection sel{mins, maxs};
	if (!sel.isValid()) {
		return false;
	}
	bool changed = false;
	for (size_t i = 0; i < _selections.size();) {
		Selection &s = _selections[i];
		if (sel.containsRegion(s)) {
			_selections.erase(i);
			changed = true;
		} else if (voxel::intersects(sel, s)) {
			const Selections newRegions = voxel::Region::subtract(s, sel);
			_selections.erase(i);
			_selections.insert(_selections.begin() + i, newRegions.begin(), newRegions.end());
			i += newRegions.size();
			changed = true;
		} else {
			++i;
		}
	}
	if (changed) {
		markDirty();
	}
	return changed;
}

bool SelectionManager::select(scenegraph::SceneGraphNode &node, const glm::ivec3 &pos) {
	if (!node.isModelNode()) {
		return false;
	}
	return select(node, pos, pos);
}

bool SelectionManager::isSelected(const glm::ivec3 &pos) const {
	if (!dirty()) {
		if (!_cachedRegion.containsPoint(pos)) {
			return false;
		}
	}

	for (const Selection &sel : _selections) {
		if (sel.containsPoint(pos)) {
			return true;
		}
	}
	return false;
}

voxel::RawVolume *SelectionManager::cut(scenegraph::SceneGraphNode &node) {
	if (!node.isModelNode()) {
		return nullptr;
	}
	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		return nullptr;
	}
	if (!hasSelection()) {
		return nullptr;
	}
	voxel::RawVolume *v = new voxel::RawVolume(*volume, _selections);
	for (const Selection &selection : _selections) {
		const glm::ivec3 &mins = selection.getLowerCorner();
		const glm::ivec3 &maxs = selection.getUpperCorner();
		static constexpr voxel::Voxel AIR;
		voxel::RawVolume::Sampler sampler(*volume);
		sampler.setPosition(mins);
		for (int32_t z = mins.z; z <= maxs.z; ++z) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int32_t y = mins.y; y <= maxs.y; ++y) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (int32_t x = mins.x; x <= maxs.x; ++x) {
					sampler3.setVoxel(AIR);
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	}
	return v;
}

voxel::RawVolume *SelectionManager::copy(const scenegraph::SceneGraphNode &node) {
	if (!node.isModelNode()) {
		return nullptr;
	}
	const voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		return nullptr;
	}
	if (!hasSelection()) {
		return nullptr;
	}
	voxel::RawVolume *v = new voxel::RawVolume(*volume, _selections);
	return v;
}

} // namespace voxedit
