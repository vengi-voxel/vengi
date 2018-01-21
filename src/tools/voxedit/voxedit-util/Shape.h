#pragma once

#include <stdint.h>

namespace voxedit {

enum class Shape : uint8_t {
	Single,
	Cone,
	Dome,
	Circle,
	Plane,
	Sphere,
	Torus,
	Cylinder
};

}
