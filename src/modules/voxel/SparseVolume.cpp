/**
 * @file
 */

#include "SparseVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxel {

SparseVolume::SparseVolume(const voxel::Region &region) : _region(region), _isRegionValid(_region.isValid()) {
}

bool SparseVolume::setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel) {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}
	if (isAir(voxel.getMaterial())) {
		_map.remove(pos);
		return true;
	}
	_map.put(pos, voxel);
	return true;
}

const Voxel &SparseVolume::voxel(const glm::ivec3 &pos) const {
	auto iter = _map.find(pos);
	if (iter != _map.end()) {
		return iter->second;
	}
	return _emptyVoxel;
}

void SparseVolume::copyTo(voxel::RawVolumeWrapper &target) const {
	for (auto iter = _map.begin(); iter != _map.end(); ++iter) {
		const glm::ivec3 &pos = iter->first;
		const voxel::Voxel &voxel = iter->second;
		target.setVoxel(pos.x, pos.y, pos.z, voxel);
	}
}

void SparseVolume::copyFrom(const voxel::RawVolume &source) {
	auto visitor = [this](int x, int y, int z, const voxel::Voxel &voxel) { setVoxel(x, y, z, voxel); };
	voxelutil::visitVolume(source, visitor);
}

SparseVolume::Sampler::Sampler(const SparseVolume *volume) : _volume(const_cast<SparseVolume *>(volume)) {
}

SparseVolume::Sampler::Sampler(const SparseVolume &volume) : _volume(const_cast<SparseVolume *>(&volume)) {
}

SparseVolume::Sampler::~Sampler() {
}

bool SparseVolume::Sampler::setVoxel(const Voxel &voxel) {
	if (_currentPositionInvalid) {
		return false;
	}
	_volume->setVoxel(_posInVolume, voxel);
	return true;
}

bool SparseVolume::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	_posInVolume.x = xPos;
	_posInVolume.y = yPos;
	_posInVolume.z = zPos;

	const voxel::Region &region = this->region();
	_currentPositionInvalid = 0u;
	if (region.isValid()) {
		if (!region.containsPointInX(xPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		}
		if (!region.containsPointInY(yPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		}
		if (!region.containsPointInZ(zPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		}
	}

	// Then we update the voxel pointer
	if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
		return true;
	}
	return false;
}

void SparseVolume::Sampler::movePositive(math::Axis axis, uint32_t offset) {
	switch (axis) {
	case math::Axis::X:
		movePositiveX(offset);
		break;
	case math::Axis::Y:
		movePositiveY(offset);
		break;
	case math::Axis::Z:
		movePositiveZ(offset);
		break;
	default:
		break;
	}
}

void SparseVolume::Sampler::movePositiveX(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.x += (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInX(_posInVolume.x)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}
	}

	// Then we update the voxel pointer
	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

void SparseVolume::Sampler::movePositiveY(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.y += (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInY(_posInVolume.y)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDY;
		}
	}

	// Then we update the voxel pointer
	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

void SparseVolume::Sampler::movePositiveZ(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.z += (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInZ(_posInVolume.z)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDZ;
		}
	}

	// Then we update the voxel pointer
	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

void SparseVolume::Sampler::moveNegative(math::Axis axis, uint32_t offset) {
	switch (axis) {
	case math::Axis::X:
		moveNegativeX(offset);
		break;
	case math::Axis::Y:
		moveNegativeY(offset);
		break;
	case math::Axis::Z:
		moveNegativeZ(offset);
		break;
	default:
		break;
	}
}

void SparseVolume::Sampler::moveNegativeX(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.x -= (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInX(_posInVolume.x)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}
	}

	// Then we update the voxel pointer
	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

void SparseVolume::Sampler::moveNegativeY(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.y -= (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInY(_posInVolume.y)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDY;
		}
	}

	// Then we update the voxel pointer
	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

void SparseVolume::Sampler::moveNegativeZ(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.z -= (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInZ(_posInVolume.z)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDZ;
		}
	}

	// Then we update the voxel pointer
	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

} // namespace voxel
