/**
 * @file
 */

#pragma once

#include "Shape.h"
#include <cstdint>
#include <string>

namespace stock {

struct ContainerData {
	std::string name;
	ContainerShape shape;
	uint32_t flags = 0u;
	uint8_t id = 0u;
};

}
