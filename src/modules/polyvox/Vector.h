#pragma once

#include "core/Common.h"
#include "LoggingImpl.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>

namespace PolyVox {

/// A 3D Vector of floats.
typedef glm::vec3 Vector3DFloat;
/// A 3D Vector of unsigned 8-bit values.
typedef glm::i8vec3 Vector3DUint8;
/// A 3D Vector of unsigned 16-bit values.
typedef glm::i16vec3 Vector3DUint16;
/// A 3D Vector of signed 32-bit values.
typedef glm::ivec3 Vector3DInt32;

}	//namespace PolyVox

namespace std {
template<>
struct hash<PolyVox::Vector3DInt32> {
	std::size_t operator()(const PolyVox::Vector3DInt32& vec) const {
		return ((vec.x & 0xFF)) | ((vec.y & 0xFF) << 8) | ((vec.z & 0xFF) << 16);
	}
};
}
