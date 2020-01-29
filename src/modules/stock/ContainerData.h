/**
 * @file
 */

#pragma once

#include "Shape.h"
#include <stdint.h>
#include "core/String.h"

namespace stock {

struct ContainerData {
	std::string name;
	ContainerShape shape;
	uint32_t flags = 0u;
	uint8_t id = 0u;
};

}
