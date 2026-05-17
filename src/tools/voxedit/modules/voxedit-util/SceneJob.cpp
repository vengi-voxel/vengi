/**
 * @file
 */

#include "SceneJob.h"
#include "app/I18N.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelutil/FillHollow.h"
#include "voxelutil/Hollow.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

static constexpr const char *SceneJobTypeText[(int)SceneJobType::Max] = {
	NC_("SceneJob", "Scene job"),
	NC_("SceneJob", "Cropping"),
	NC_("SceneJob", "Scaling up"),
	NC_("SceneJob", "Scaling down"),
	NC_("SceneJob", "Resizing"),
	NC_("SceneJob", "Filling"),
	NC_("SceneJob", "Clearing"),
	NC_("SceneJob", "Deleting selected voxels"),
	NC_("SceneJob", "Hollowing"),
	NC_("SceneJob", "Filling hollow voxels"),
	NC_("SceneJob", "Splitting objects"),
	NC_("SceneJob", "Moving color to model"),
	NC_("SceneJob", "Splat merging")
};
static_assert(lengthof(SceneJobTypeText) == (int)SceneJobType::Max, "SceneJobTypeText size mismatch");

const char *sceneJobText(SceneJobType type) {
	const int index = (int)type;
	if (index <= (int)SceneJobType::None || index >= (int)SceneJobType::Max) {
		return _(SceneJobTypeText[(int)SceneJobType::None]);
	}
	return _(SceneJobTypeText[index]);
}

SceneJobSplatTarget::~SceneJobSplatTarget() {
	delete volume;
	volume = nullptr;
}

SceneJobSplatTarget::SceneJobSplatTarget(const SceneJobSplatTarget &other) {
	*this = other;
}

SceneJobSplatTarget &SceneJobSplatTarget::operator=(const SceneJobSplatTarget &other) {
	if (this != &other) {
		delete volume;
		nodeId = other.nodeId;
		volume = other.volume != nullptr ? new voxel::RawVolume(*other.volume) : nullptr;
		palette = other.palette;
		worldRegion = other.worldRegion;
	}
	return *this;
}

SceneJobSplatTarget::SceneJobSplatTarget(SceneJobSplatTarget &&other) noexcept {
	*this = core::move(other);
}

SceneJobSplatTarget &SceneJobSplatTarget::operator=(SceneJobSplatTarget &&other) noexcept {
	if (this != &other) {
		delete volume;
		nodeId = other.nodeId;
		volume = other.volume;
		palette = other.palette;
		worldRegion = other.worldRegion;
		other.nodeId = InvalidNodeId;
		other.volume = nullptr;
		other.worldRegion = voxel::Region::InvalidRegion;
	}
	return *this;
}

SceneJobVolumeResult::~SceneJobVolumeResult() {
	delete volume;
	volume = nullptr;
}

SceneJobVolumeResult::SceneJobVolumeResult(const SceneJobVolumeResult &other) {
	*this = other;
}

SceneJobVolumeResult &SceneJobVolumeResult::operator=(const SceneJobVolumeResult &other) {
	if (this != &other) {
		delete volume;
		nodeId = other.nodeId;
		volume = other.volume != nullptr ? new voxel::RawVolume(*other.volume) : nullptr;
		modifiedRegion = other.modifiedRegion;
	}
	return *this;
}

SceneJobVolumeResult::SceneJobVolumeResult(SceneJobVolumeResult &&other) noexcept {
	*this = core::move(other);
}

SceneJobVolumeResult &SceneJobVolumeResult::operator=(SceneJobVolumeResult &&other) noexcept {
	if (this != &other) {
		delete volume;
		nodeId = other.nodeId;
		volume = other.volume;
		modifiedRegion = other.modifiedRegion;

		other.nodeId = InvalidNodeId;
		other.volume = nullptr;
		other.modifiedRegion = voxel::Region::InvalidRegion;
	}
	return *this;
}

voxel::RawVolume *SceneJobVolumeResult::releaseVolume() {
	voxel::RawVolume *v = volume;
	volume = nullptr;
	return v;
}

SceneJobNewNode::~SceneJobNewNode() {
	delete volume;
	volume = nullptr;
}

SceneJobNewNode::SceneJobNewNode(const SceneJobNewNode &other) {
	*this = other;
}

SceneJobNewNode &SceneJobNewNode::operator=(const SceneJobNewNode &other) {
	if (this != &other) {
		delete volume;
		sourceNodeId = other.sourceNodeId;
		parentNodeId = other.parentNodeId;
		name = other.name;
		palette = other.palette;
		volume = other.volume != nullptr ? new voxel::RawVolume(*other.volume) : nullptr;
	}
	return *this;
}

SceneJobNewNode::SceneJobNewNode(SceneJobNewNode &&other) noexcept {
	*this = core::move(other);
}

SceneJobNewNode &SceneJobNewNode::operator=(SceneJobNewNode &&other) noexcept {
	if (this != &other) {
		delete volume;
		sourceNodeId = other.sourceNodeId;
		parentNodeId = other.parentNodeId;
		name = core::move(other.name);
		palette = other.palette;
		volume = other.volume;

		other.sourceNodeId = InvalidNodeId;
		other.parentNodeId = InvalidNodeId;
		other.volume = nullptr;
	}
	return *this;
}

voxel::RawVolume *SceneJobNewNode::releaseVolume() {
	voxel::RawVolume *v = volume;
	volume = nullptr;
	return v;
}

SceneJobResult::~SceneJobResult() {
	delete volume;
	volume = nullptr;
}

SceneJobResult::SceneJobResult(SceneJobResult &&other) noexcept {
	*this = core::move(other);
}

SceneJobResult &SceneJobResult::operator=(SceneJobResult &&other) noexcept {
	if (this != &other) {
		delete volume;
		type = other.type;
		nodeId = other.nodeId;
		volume = other.volume;
		modifiedRegion = other.modifiedRegion;
		volumes = core::move(other.volumes);
		newNodes = core::move(other.newNodes);
		removeSourceNode = other.removeSourceNode;
		success = other.success;
		error = core::move(other.error);

		other.type = SceneJobType::None;
		other.nodeId = InvalidNodeId;
		other.volume = nullptr;
		other.modifiedRegion = voxel::Region::InvalidRegion;
		other.removeSourceNode = false;
		other.success = false;
	}
	return *this;
}

voxel::RawVolume *SceneJobResult::releaseVolume() {
	voxel::RawVolume *v = volume;
	volume = nullptr;
	return v;
}

voxel::Region sceneJobModifiedRegionForResize(const voxel::Region &oldRegion, const voxel::Region &newRegion) {
	return voxel::Region(glm::min(oldRegion.getLowerCorner(), newRegion.getLowerCorner()),
						 glm::max(oldRegion.getUpperCorner(), newRegion.getUpperCorner()));
}

SceneJobResult makeVolumeOperationSceneJobResult(SceneJobType type, int nodeId, voxel::RawVolume *snapshot,
												 const voxel::Region &selectionRegion, const voxel::Voxel &voxel,
												 bool overrideVoxels) {
	SceneJobResult result;
	result.type = type;
	result.nodeId = nodeId;

	voxel::RawVolumeWrapper wrapper(snapshot);
	switch (type) {
	case SceneJobType::FillVolume:
		voxelutil::fill(wrapper, voxel, overrideVoxels);
		break;
	case SceneJobType::ClearVolume:
		voxelutil::clear(wrapper);
		break;
	case SceneJobType::DeleteSelectedVolume:
		if (!selectionRegion.isValid()) {
			result.error = "No selected voxels to delete";
			delete snapshot;
			return result;
		}
		voxelutil::visitVolume(*snapshot, selectionRegion, [&](int x, int y, int z, const voxel::Voxel &) {
			wrapper.setVoxel(x, y, z, voxel::Voxel());
		}, voxelutil::VisitSolidOutline());
		break;
	case SceneJobType::HollowVolume:
		voxelutil::hollow(wrapper);
		break;
	case SceneJobType::FillHollowVolume:
		voxelutil::fillHollow(wrapper, voxel);
		break;
	default:
		result.error = "Unsupported volume operation";
		delete snapshot;
		return result;
	}

	result.volume = snapshot;
	result.modifiedRegion = wrapper.dirtyRegion();
	result.success = true;
	return result;
}

} // namespace voxedit
