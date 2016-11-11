#pragma once

#include <cstdint>

namespace voxedit {

enum class Shape : uint8_t {
	Single,
	Cone,
	Dome,
	Circle,
	Plane,
	Sphere
};

}
