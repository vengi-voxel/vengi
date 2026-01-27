/**
 * @file
 */

#include "SelectionManager.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"

namespace voxedit {

void SelectionManager::reset() {
	markClean();
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
	scenegraph::Selections &selections = node.selections();
	if (selections.empty()) {
		select(node, volume->region().getLowerCorner(), volume->region().getUpperCorner());
	} else {
		const scenegraph::Selections &remainingSelections = voxel::Region::subtract(volume->region(), selections);
		node.clearSelections();
		for (const voxel::Region &selection : remainingSelections) {
			select(node, selection.getLowerCorner(), selection.getUpperCorner());
		}
	}
}

void SelectionManager::unselect(scenegraph::SceneGraphNode &node) {
	if (!node.isModelNode()) {
		return;
	}
	node.clearSelections();
}

voxel::Region SelectionManager::calculateRegion(const scenegraph::SceneGraphNode &node) const {
	const scenegraph::Selections &selections = node.selections();
	if (selections.empty()) {
		return voxel::Region::InvalidRegion;
	}
	voxel::Region r = selections[0];
	for (const voxel::Region &selection : selections) {
		r.accumulate(selection);
	}
	return r;
}

bool SelectionManager::select(scenegraph::SceneGraphNode &node, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
	if (!node.isModelNode()) {
		return false;
	}
	const voxel::Region sel{mins, maxs};
	if (!sel.isValid()) {
		return false;
	}
	scenegraph::Selections &selections = node.selections();
	for (size_t i = 0; i < selections.size(); ++i) {
		const voxel::Region &s = selections[i];
		if (s.containsRegion(sel)) {
			return true;
		}
	}

	for (size_t i = 0; i < selections.size();) {
		voxel::Region &s = selections[i];
		if (sel.containsRegion(s)) {
			selections.erase(i);
		} else if (voxel::intersects(sel, s)) {
			const scenegraph::Selections newRegions = voxel::Region::subtract(s, sel);
			selections.erase(i);
			selections.insert(selections.begin() + i, newRegions.begin(), newRegions.end());
			i += newRegions.size();
		} else {
			++i;
		}
	}
	selections.push_back(sel);
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
	const voxel::Region sel{mins, maxs};
	if (!sel.isValid()) {
		return false;
	}
	scenegraph::Selections &selections = node.selections();
	bool changed = false;
	for (size_t i = 0; i < selections.size();) {
		voxel::Region &s = selections[i];
		if (sel.containsRegion(s)) {
			selections.erase(i);
			changed = true;
		} else if (voxel::intersects(sel, s)) {
			const scenegraph::Selections newRegions = voxel::Region::subtract(s, sel);
			selections.erase(i);
			selections.insert(selections.begin() + i, newRegions.begin(), newRegions.end());
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

bool SelectionManager::isSelected(const scenegraph::SceneGraphNode &node, const glm::ivec3 &pos) const {
	const scenegraph::Selections &selections = node.selections();
	for (const voxel::Region &sel : selections) {
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
	const scenegraph::Selections &selections = node.selections();
	if (selections.empty()) {
		return nullptr;
	}
	voxel::RawVolume *v = new voxel::RawVolume(*volume, selections);
	for (const voxel::Region &selection : selections) {
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
	const scenegraph::Selections &selections = node.selections();
	if (selections.empty()) {
		return nullptr;
	}
	voxel::RawVolume *v = new voxel::RawVolume(*volume, selections);
	return v;
}

} // namespace voxedit
