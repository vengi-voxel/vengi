/**
 * @file
 */

#pragma once

#include "BrushApply.h"
#include "ModifierVolumeWrapper.h"
#include "core/ScopedPtr.h"
#include "core/SharedPtr.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxelutil/VolumeVisitor.h"
#include <string.h>

namespace voxedit {

namespace priv {

inline void copyView(scenegraph::SceneGraph &view, const scenegraph::SceneGraph &source,
					 const scenegraph::SceneGraphNode &parent, int parentId) {
	for (const core::UUID &uuid : parent.children()) {
		const scenegraph::SceneGraphNode *node = source.findNodeByUUID(uuid);
		if (!node) {
			continue;
		}
		scenegraph::SceneGraphNode copy(node->type(), uuid);
		scenegraph::copyNode(*node, copy, false);
		const int id = view.emplace(core::move(copy), parentId);
		copyView(view, source, *node, id);
	}
}

inline void accumulate(voxel::Region &dest, const voxel::Region &region) {
	if (region.isValid()) {
		if (dest.isValid()) {
			dest.accumulate(region);
		} else {
			dest = region;
		}
	}
}

} // namespace priv

/** Isolated adjustable operation. No input, widget, renderer, or memento ownership. */
template<class Operation>
class DraftEditSession {
private:
	struct Target {
		core::UUID uuid;
		const voxel::RawVolume *identity = nullptr;
		uint64_t paletteHash = 0;
		uint32_t normalPaletteHash = 0;
		palette::Palette palette;
		palette::NormalPalette normalPalette;
		bool paletteChanged = false;
		bool normalPaletteChanged = false;
		core::SharedPtr<voxel::RawVolume> original;
		core::SharedPtr<voxel::RawVolume> draft;
		Operation operation;
		voxel::Region dirty = voxel::Region::InvalidRegion;
	};
	core::DynamicArray<core::SharedPtr<Target>> _targets;
	core::ScopedPtr<scenegraph::SceneGraph> _view;
	typename Operation::Parameters _parameters;
	voxel::Region _constraint = voxel::Region::InvalidRegion;
	uint64_t _revision = 0;

public:
	bool active() const {
		return !_targets.empty();
	}
	uint64_t revision() const {
		return _revision;
	}
	const typename Operation::Parameters &parameters() const {
		return _parameters;
	}
	void clear();
	bool begin(const scenegraph::SceneGraph &scene, const core::Buffer<core::UUID> &targets,
			   const voxel::Region &constraint = voxel::Region::InvalidRegion,
			   const typename Operation::Parameters &parameters = {});
	bool valid(const scenegraph::SceneGraph &scene, bool checkContent = false) const;
	BrushApplyResult update(const typename Operation::Parameters &parameters, const BrushApplyOptions &options = {});
	BrushApplyResult apply(scenegraph::SceneGraph &scene);
	const scenegraph::SceneGraph *view() const {
		return _view;
	}
	voxel::Region dirtyRegion(const core::UUID &uuid) const;
	bool paletteChanged(const core::UUID &uuid) const;
	bool normalPaletteChanged(const core::UUID &uuid) const;
};

template<class Operation>
void DraftEditSession<Operation>::clear() {
	_view = nullptr;
	_targets.clear();
	++_revision;
}

template<class Operation>
bool DraftEditSession<Operation>::begin(const scenegraph::SceneGraph &scene, const core::Buffer<core::UUID> &targets,
										const voxel::Region &constraint,
										const typename Operation::Parameters &parameters) {
	clear();
	_constraint = constraint;
	_parameters = parameters;
	_view = new scenegraph::SceneGraph();
	_view->setAnimations(scene.animations());
	scenegraph::copyNode(scene.root(), _view->node(0), false);
	priv::copyView(*_view, scene, scene.root(), 0);
	_view->setAnimation(scene.activeAnimation());
	_view->setActiveNode(scene.activeNodeUUID());
	for (const core::UUID &uuid : targets) {
		const scenegraph::SceneGraphNode *node = scene.findNodeByUUID(uuid);
		if (!node || !node->isModelNode() || !node->volume() ||
			(Operation::requiresSelection && !node->hasSelection())) {
			continue;
		}
		bool duplicate = false;
		for (const auto &existing : _targets) {
			duplicate |= existing->uuid == uuid || existing->identity == node->volume();
		}
		if (duplicate) {
			continue;
		}
		auto target = core::make_shared<Target>();
		target->uuid = uuid;
		target->identity = node->volume();
		target->palette = node->palette();
		target->normalPalette = node->normalPalette();
		target->paletteHash = node->palette().hash();
		target->normalPaletteHash = node->normalPalette().hash();
		target->original = core::make_shared<voxel::RawVolume>(*node->volume());
		target->draft = core::make_shared<voxel::RawVolume>(*node->volume());
		voxel::Region captureRegion = node->region();
		if (constraint.isValid() && !captureRegion.cropTo(constraint)) {
			continue;
		}
		target->operation.setParameters(_parameters);
		target->operation.prepare(target->original.get(), captureRegion);
		if (!target->operation.hasSnapshot()) {
			continue;
		}
		_view->findNodeByUUID(uuid)->setUnownedVolume(target->draft.get());
		_targets.push_back(target);
	}
	if (_targets.empty()) {
		clear();
		return false;
	}
	return true;
}

template<class Operation>
bool DraftEditSession<Operation>::valid(const scenegraph::SceneGraph &scene, bool checkContent) const {
	if (!active() || !_view || scene.nodeSize() != _view->nodeSize()) {
		return false;
	}
	for (const auto &target : _targets) {
		const scenegraph::SceneGraphNode *node = scene.findNodeByUUID(target->uuid);
		if (!node || node->palette().hash() != target->paletteHash ||
			node->normalPalette().hash() != target->normalPaletteHash || node->volume() != target->identity ||
			node->region() != target->original->region() ||
			(checkContent &&
			 memcmp(node->volume()->data(), target->original->data(), voxel::RawVolume::size(node->region())) != 0)) {
			return false;
		}
	}
	return true;
}

template<class Operation>
BrushApplyResult DraftEditSession<Operation>::update(const typename Operation::Parameters &parameters,
													 const BrushApplyOptions &options) {
	BrushApplyResult result;
	Operation validator;
	if (!active() || !validator.setParameters(parameters)) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	_parameters = validator.parameters();
	for (const auto &target : _targets) {
		target->operation.setParameters(_parameters);
		voxel::Region inputRegion = target->original->region();
		if (_constraint.isValid()) {
			inputRegion.cropTo(_constraint);
		}
		target->operation.prepare(target->original.get(), inputRegion);
		scenegraph::SceneGraphNode *node = _view->findNodeByUUID(target->uuid);
		// Each evaluation derives from the captured input, including removals.
		const voxel::Region previousDirty = target->dirty;
		if (previousDirty.isValid()) {
			target->draft->copyInto(*target->original.get(), previousDirty);
		}
		node->setPalette(target->palette);
		node->setNormalPalette(target->normalPalette);
		ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
		if (previousDirty.isValid()) {
			wrapper.addToDirtyRegion(previousDirty.getLowerCorner());
			wrapper.addToDirtyRegion(previousDirty.getUpperCorner());
		}
		target->operation.evaluate(wrapper);
		if (options.applySelectionPolicy) {
			if (options.autoSelect) {
				wrapper.autoSelectNewVoxels();
			} else {
				wrapper.growSelectionToNewVoxels();
			}
		}
		target->paletteChanged = node->palette().hash() != target->paletteHash;
		target->normalPaletteChanged = node->normalPalette().hash() != target->normalPaletteHash;
		result.paletteChanged |= target->paletteChanged;
		result.normalPaletteChanged |= target->normalPaletteChanged;
		// History and redraw bounds are independent of whether the algorithm touched a cell.
		target->dirty = voxel::Region::InvalidRegion;
		voxel::Region changedRegion = wrapper.dirtyRegion();
		if (!changedRegion.isValid()) {
			continue;
		}
		changedRegion.cropTo(target->original->region());
		voxelutil::visitVolume(
			*target->original.get(), changedRegion,
			[&](int x, int y, int z, const voxel::Voxel &old) {
				if (_constraint.isValid() && !_constraint.containsPoint(glm::ivec3(x, y, z))) {
					target->draft->setVoxel(x, y, z, old);
				} else if (!(old == target->draft->voxel(x, y, z))) {
					priv::accumulate(target->dirty, voxel::Region(x, y, z, x, y, z));
				}
			},
			voxelutil::VisitAll());
		priv::accumulate(result.dirtyRegion, target->dirty);
	}
	++_revision;
	result.status = result.changed() ? BrushApplyStatus::Changed : BrushApplyStatus::Unchanged;
	return result;
}

template<class Operation>
BrushApplyResult DraftEditSession<Operation>::apply(scenegraph::SceneGraph &scene) {
	BrushApplyResult result;
	if (!valid(scene, true)) {
		result.status = BrushApplyStatus::InvalidTarget;
		return result;
	}
	// Every target was validated before publishing any writes. Region and allocation stay unchanged.
	for (const auto &target : _targets) {
		if (target->paletteChanged) {
			scene.findNodeByUUID(target->uuid)->setPalette(_view->findNodeByUUID(target->uuid)->palette());
			result.paletteChanged = true;
		}
		if (target->normalPaletteChanged) {
			scene.findNodeByUUID(target->uuid)->setNormalPalette(_view->findNodeByUUID(target->uuid)->normalPalette());
			result.normalPaletteChanged = true;
		}
		if (target->dirty.isValid()) {
			scene.findNodeByUUID(target->uuid)->volume()->copyInto(*target->draft.get(), target->dirty);
			priv::accumulate(result.dirtyRegion, target->dirty);
		}
	}
	result.status = result.changed() ? BrushApplyStatus::Changed : BrushApplyStatus::Unchanged;
	return result;
}

template<class Operation>
voxel::Region DraftEditSession<Operation>::dirtyRegion(const core::UUID &uuid) const {
	for (const auto &target : _targets) {
		if (target->uuid == uuid) {
			return target->dirty;
		}
	}
	return voxel::Region::InvalidRegion;
}

template<class Operation>
bool DraftEditSession<Operation>::paletteChanged(const core::UUID &uuid) const {
	for (const auto &target : _targets) {
		if (target->uuid == uuid) {
			return target->paletteChanged;
		}
	}
	return false;
}
template<class Operation>
bool DraftEditSession<Operation>::normalPaletteChanged(const core::UUID &uuid) const {
	for (const auto &target : _targets) {
		if (target->uuid == uuid) {
			return target->normalPaletteChanged;
		}
	}
	return false;
}

} // namespace voxedit
