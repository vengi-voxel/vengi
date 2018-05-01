/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace voxedit {

enum class SelectType : uint8_t {
	Single,
	Same,
	LineVertical,
	LineHorizontal,
	Edge,
	AABB,

	Max
};

}
