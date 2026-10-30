/**
 * @file
 */

#include "BrushEditSession.h"
#include "core/Common.h"

namespace voxedit {

void BrushEditSession::reset() {
	_active = false;
	_sliceAction = false;
	_hasOverride = false;
	_awaitingSecondAction = false;
	_brushType = BrushType::None;
	_resolvedOperation = ModifierType::None;
	_persistentOperation = ModifierType::None;
	_stroke.reset();
	_targets.clear();
}

bool BrushEditSession::begin(BrushType brushType, ModifierType resolvedOperation, ModifierType persistentOperation,
							 bool hasOverride, bool sliceAction) {
	reset();
	++_id;
	_active = true;
	_sliceAction = sliceAction;
	_hasOverride = hasOverride;
	_brushType = brushType;
	_resolvedOperation = resolvedOperation;
	_persistentOperation = persistentOperation;
	_stroke.begin();
	return true;
}

void BrushEditSession::end() {
	_stroke.reset();
	_targets.clear();
	_active = false;
	_hasOverride = false;
	_awaitingSecondAction = false;
	_sliceAction = false;
	_brushType = BrushType::None;
	_resolvedOperation = ModifierType::None;
	_persistentOperation = ModifierType::None;
}

void BrushEditSession::addTarget(const core::UUID &nodeUUID, voxel::RawVolume *volume) {
	if (!volume) {
		return;
	}
	for (const BrushEditTarget &existing : _targets) {
		if (existing.volume == volume || existing.nodeUUID == nodeUUID) {
			return;
		}
	}
	BrushEditTarget target;
	target.nodeUUID = nodeUUID;
	target.volume = volume;
	_targets.push_back(core::move(target));
}

BrushEditTarget *BrushEditSession::findTarget(const core::UUID &nodeUUID) {
	for (BrushEditTarget &target : _targets) {
		if (target.nodeUUID == nodeUUID) {
			return &target;
		}
	}
	return nullptr;
}

const BrushEditTarget *BrushEditSession::findTarget(const core::UUID &nodeUUID) const {
	for (const auto &target : _targets) {
		if (target.nodeUUID == nodeUUID) {
			return &target;
		}
	}
	return nullptr;
}

void BrushEditSession::accumulate(BrushEditTarget &target, const voxel::Region &region) {
	if (!region.isValid()) {
		return;
	}
	if (target.dirtyRegion.isValid()) {
		target.dirtyRegion.accumulate(region);
	} else {
		target.dirtyRegion = region;
	}
}

bool BrushEditSession::hasChangedTargets() const {
	for (const BrushEditTarget &target : _targets) {
		if (target.dirtyRegion.isValid()) {
			return true;
		}
	}
	return false;
}

} // namespace voxedit
