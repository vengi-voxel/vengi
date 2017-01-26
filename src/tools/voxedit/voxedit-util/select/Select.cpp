#include "Select.h"

namespace voxedit {
namespace selections {

Select::~Select() {
}

void Select::goUp(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const {
	model.movePositiveY();
	if (!model.isCurrentPositionValid()) {
		return;
	}
	selection.movePositiveY();
	const bool ret = selection.setVoxel(model.getVoxel());
	if (ret) {
		++cnt;
	}
	goUp(model, selection, cnt);
}

void Select::goDown(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const {
	model.moveNegativeY();
	if (!model.isCurrentPositionValid()) {
		return;
	}
	selection.moveNegativeY();
	const bool ret = selection.setVoxel(model.getVoxel());
	if (ret) {
		++cnt;
	}
	goDown(model, selection, cnt);
}

void Select::goLeft(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const {
	model.moveNegativeX();
	if (!model.isCurrentPositionValid()) {
		return;
	}
	selection.moveNegativeX();
	const bool ret = selection.setVoxel(model.getVoxel());
	if (ret) {
		++cnt;
	}
	goLeft(model, selection, cnt);
}

void Select::goRight(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const {
	model.movePositiveX();
	if (!model.isCurrentPositionValid()) {
		return;
	}
	selection.movePositiveX();
	const bool ret = selection.setVoxel(model.getVoxel());
	if (ret) {
		++cnt;
	}
	goRight(model, selection, cnt);
}

bool Select::sixDirectionsExecute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel& voxel, int& cnt) const {
	if (!model.isCurrentPositionValid()) {
		return false;
	}
	const voxel::Voxel& v = model.getVoxel();
	if (v == voxel) {
		selection.setPosition(model.getPosition());
		if (selection.setVoxel(voxel)) {
			++cnt;
			goSixDirections(model, selection, voxel, cnt);
			return true;
		}
	}
	return false;
}

void Select::goSixDirections(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel voxel, int& cnt) const {
	const glm::ivec3& pos = model.getPosition();

	model.moveNegativeX();
	sixDirectionsExecute(model, selection, voxel, cnt);
	model.setPosition(pos);

	model.moveNegativeY();
	selection.setPosition(model.getPosition());
	if (!isBlocked(selection.getVoxel().getMaterial())) {
		sixDirectionsExecute(model, selection, voxel, cnt);
	}
	model.setPosition(pos);

	model.moveNegativeZ();
	selection.setPosition(model.getPosition());
	if (!isBlocked(selection.getVoxel().getMaterial())) {
		sixDirectionsExecute(model, selection, voxel, cnt);
	}
	model.setPosition(pos);

	model.movePositiveX();
	selection.setPosition(model.getPosition());
	if (!isBlocked(selection.getVoxel().getMaterial())) {
		sixDirectionsExecute(model, selection, voxel, cnt);
	}
	model.setPosition(pos);

	model.movePositiveY();
	selection.setPosition(model.getPosition());
	if (!isBlocked(selection.getVoxel().getMaterial())) {
		sixDirectionsExecute(model, selection, voxel, cnt);
	}
	model.setPosition(pos);

	model.movePositiveZ();
	selection.setPosition(model.getPosition());
	if (!isBlocked(selection.getVoxel().getMaterial())) {
		sixDirectionsExecute(model, selection, voxel, cnt);
	}

	selection.setPosition(pos);
	model.setPosition(pos);
}

int Select::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	return selection.setVoxel(model.getVoxel()) ? 1 : 0;
}

int Select::execute(const voxel::RawVolume *model, voxel::RawVolume *selection, const glm::ivec3& pos) const {
	if (!model->getRegion().containsPoint(pos)) {
		Log::error("Given position is outside of the region");
		return 0;
	}
	voxel::RawVolume::Sampler m(model);

	m.setPosition(pos);
	const voxel::Voxel& currentVoxel = m.getVoxel();
	if (!isBlocked(currentVoxel.getMaterial())) {
		return 0;
	}
	voxel::RawVolume::Sampler s(selection);
	s.setPosition(pos);
	return execute(m, s);
}

}
}
