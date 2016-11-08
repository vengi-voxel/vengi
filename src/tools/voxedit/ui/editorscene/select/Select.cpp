#include "Select.h"

namespace voxedit {
namespace selections {

Select::~Select() {
}

void Select::goUp(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	model.movePositiveY();
	if (!model.isCurrentPositionValid()) {
		return;
	}
	selection.movePositiveY();
	selection.setVoxel(model.getVoxel());
	goUp(model, selection);
}

void Select::goDown(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	model.moveNegativeY();
	if (!model.isCurrentPositionValid()) {
		return;
	}
	selection.moveNegativeY();
	selection.setVoxel(model.getVoxel());
	goDown(model, selection);
}

void Select::goLeft(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	model.moveNegativeX();
	if (!model.isCurrentPositionValid()) {
		return;
	}
	selection.moveNegativeX();
	selection.setVoxel(model.getVoxel());
	goLeft(model, selection);
}

void Select::goRight(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	model.movePositiveX();
	if (!model.isCurrentPositionValid()) {
		return;
	}
	selection.movePositiveX();
	selection.setVoxel(model.getVoxel());
	goRight(model, selection);
}

bool Select::sixDirectionsExecute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel& voxel) const {
	if (!model.isCurrentPositionValid()) {
		return false;
	}
	const voxel::Voxel& v = model.getVoxel();
	if (v == voxel) {
		selection.setPosition(model.getPosition());
		if (selection.setVoxel(voxel)) {
			goSixDirections(model, selection, voxel);
			return true;
		}
	}
	return false;
}

void Select::goSixDirections(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel voxel) const {
	const glm::ivec3& pos = model.getPosition();

	model.moveNegativeX();
	sixDirectionsExecute(model, selection, voxel);
	model.setPosition(pos);

	model.moveNegativeY();
	selection.setPosition(model.getPosition());
	if (selection.getVoxel().getMaterial() == voxel::VoxelType::Air) {
		sixDirectionsExecute(model, selection, voxel);
	}
	model.setPosition(pos);

	model.moveNegativeZ();
	selection.setPosition(model.getPosition());
	if (selection.getVoxel().getMaterial() == voxel::VoxelType::Air) {
		sixDirectionsExecute(model, selection, voxel);
	}
	model.setPosition(pos);

	model.movePositiveX();
	selection.setPosition(model.getPosition());
	if (selection.getVoxel().getMaterial() == voxel::VoxelType::Air) {
		sixDirectionsExecute(model, selection, voxel);
	}
	model.setPosition(pos);

	model.movePositiveY();
	selection.setPosition(model.getPosition());
	if (selection.getVoxel().getMaterial() == voxel::VoxelType::Air) {
		sixDirectionsExecute(model, selection, voxel);
	}
	model.setPosition(pos);

	model.movePositiveZ();
	selection.setPosition(model.getPosition());
	if (selection.getVoxel().getMaterial() == voxel::VoxelType::Air) {
		sixDirectionsExecute(model, selection, voxel);
	}

	selection.setPosition(pos);
	model.setPosition(pos);
}

bool Select::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	return selection.setVoxel(model.getVoxel());
}

bool Select::execute(const voxel::RawVolume *model, voxel::RawVolume *selection, const glm::ivec3& pos) const {
	if (!model->getEnclosingRegion().containsPoint(pos)) {
		Log::error("Given position is outside of the region");
		return false;
	}
	voxel::RawVolume::Sampler m(model);

	m.setPosition(pos);
	const voxel::Voxel& currentVoxel = m.getVoxel();
	if (currentVoxel.getMaterial() == voxel::VoxelType::Air) {
		return false;
	}
	voxel::RawVolume::Sampler s(selection);
	s.setVoxel(currentVoxel);
	return execute(m, s);
}

}
}
