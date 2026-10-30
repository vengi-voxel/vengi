/**
 * @file
 */

#pragma once

#include "ModifierType.h"
#include "brush/Brush.h"
#include "brush/BrushType.h"
#include "brush/gesture/StrokeGesture.h"
#include "core/SharedPtr.h"
#include "core/UUID.h"
#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"

namespace voxedit {

enum class BrushEditFinishReason { Release, ToolChange, OperationChange, SettingsChange, SliceChange, Interrupt, ExternalInvalidate };

/**
 * @brief Captured writable target for one edit. Identity is the node UUID plus the
 * volume pointer resolved at begin; a later replacement volume must not inherit this record.
 */
struct BrushEditTarget {
	core::UUID nodeUUID;
	voxel::RawVolume *volume = nullptr;
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;
	core::SharedPtr<Brush> evaluator;
	bool paletteChanged = false;
	bool normalPaletteChanged = false;
};

/**
 * @brief Owns one Idle/Active edit: captured targets, stroke sampling, per-target dirty
 * records, and temporary operation overrides. Does not own brush algorithms or memento storage.
 */
class BrushEditSession {
private:
	bool _active = false;
	uint64_t _id = 0;
	bool _sliceAction = false;
	bool _hasOverride = false;
	bool _awaitingSecondAction = false;
	BrushType _brushType = BrushType::None;
	ModifierType _resolvedOperation = ModifierType::None;
	ModifierType _persistentOperation = ModifierType::None;
	StrokeGesture _stroke;
	core::DynamicArray<BrushEditTarget> _targets;

public:
	uint64_t id() const {
		return _id;
	}
	void reset();
	bool begin(BrushType brushType, ModifierType resolvedOperation, ModifierType persistentOperation, bool hasOverride,
			   bool sliceAction);
	void end();
	bool isActive() const {
		return _active;
	}
	bool sliceAction() const {
		return _sliceAction;
	}
	bool hasOverride() const {
		return _hasOverride;
	}
	bool awaitingSecondAction() const {
		return _awaitingSecondAction;
	}
	void setAwaitingSecondAction(bool awaiting) {
		_awaitingSecondAction = awaiting;
	}
	BrushType brushType() const {
		return _brushType;
	}
	ModifierType resolvedOperation() const {
		return _resolvedOperation;
	}
	ModifierType persistentOperation() const {
		return _persistentOperation;
	}

	StrokeGesture &stroke() {
		return _stroke;
	}
	const StrokeGesture &stroke() const {
		return _stroke;
	}

	core::DynamicArray<BrushEditTarget> &targets() {
		return _targets;
	}
	const core::DynamicArray<BrushEditTarget> &targets() const {
		return _targets;
	}

	void addTarget(const core::UUID &nodeUUID, voxel::RawVolume *volume);
	BrushEditTarget *findTarget(const core::UUID &nodeUUID);
	const BrushEditTarget *findTarget(const core::UUID &nodeUUID) const;
	void accumulate(BrushEditTarget &target, const voxel::Region &region);
	bool hasChangedTargets() const;
};

} // namespace voxedit
