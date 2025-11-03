/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicStringMap.h"
#include "scenegraph/SceneGraphTransform.h"

namespace scenegraph {

/**
 * @ingroup SceneGraph
 */
enum class InterpolationType : uint8_t {
	Instant = 0,
	Linear = 1,
	QuadEaseIn = 2,
	QuadEaseOut = 3,
	QuadEaseInOut = 4,
	CubicEaseIn = 5,
	CubicEaseOut = 6,
	CubicEaseInOut = 7,
	CubicBezier = 8,
	CatmullRom = 9,
	Max
};

static constexpr const char *InterpolationTypeStr[]{"Instant",		 "Linear",		"QuadEaseIn",	"QuadEaseOut",
													"QuadEaseInOut", "CubicEaseIn", "CubicEaseOut", "CubicEaseInOut",
													"CubicBezier",	 "CatmullRom"};
static_assert(int(scenegraph::InterpolationType::Max) == lengthof(InterpolationTypeStr), "Array sizes don't match Max");

/**
 * @ingroup SceneGraph
 */
class SceneGraphKeyFrame {
private:
	SceneGraphTransform _transform;

public:
	FrameIndex frameIdx = 0;
	InterpolationType interpolation = InterpolationType::Linear;
	// Support this by negation of the quaternions - they are equivalent, but interpolating between
	// ones of different polarity takes the longer path
	// if longRotation is true the dot of the quaternion should be > 0 otherwise < 0
	bool longRotation = false;

	inline void setTransform(const SceneGraphTransform &transform) {
		_transform = transform;
	}

	inline SceneGraphTransform &transform() {
		return _transform;
	}

	inline const SceneGraphTransform &transform() const {
		return _transform;
	}
};
using SceneGraphKeyFrames = core::Buffer<SceneGraphKeyFrame, 4>;
using SceneGraphKeyFramesMap = core::DynamicStringMap<SceneGraphKeyFrames>;

}; // namespace scenegraph
