/**
 * @file
 */

#pragma once

#include "math/CoordinateSystem.h"
#include "voxel/Voxel.h"
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace voxel {

template<class Volume>
class CoordinateSystemVolume {
protected:
	const math::CoordinateSystem _coordinateSystem;
	Volume &_volume;

	const glm::ivec3 _mins;
	const glm::ivec3 _maxs;

public:
	CoordinateSystemVolume(math::CoordinateSystem system, Volume &volume)
		: _coordinateSystem(system), _volume(volume), _mins(_volume.region().getLowerCorner()),
		  _maxs(_volume.region().getUpperCorner()) {
	}

	/**
	 * @brief Convert the coordinates into the vengi volume space
	 */
	inline bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
		glm::ivec3 v(x, y, z);
		if (!_volume.region().containsPoint(v)) {
			return false;
		}
		const int zFlipped = _maxs.z - (v.z - _mins.z);
		switch (_coordinateSystem) {
		case math::CoordinateSystem::Vengi:
		case math::CoordinateSystem::Maya:
		case math::CoordinateSystem::OpenGL:
			break;
		case math::CoordinateSystem::DirectX:
			v.z = zFlipped;
			break;
		case math::CoordinateSystem::Autodesk3dsmax:
		case math::CoordinateSystem::MagicaVoxel:
		case math::CoordinateSystem::VXL:
			v.z = v.y;
			v.y = zFlipped;
			break;
		case math::CoordinateSystem::Max:
		default:
			return false;
		}
		return _volume.setVoxel(v, voxel);
	}

	inline bool setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}
};

} // namespace voxel
