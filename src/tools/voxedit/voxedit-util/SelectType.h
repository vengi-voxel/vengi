/**
 * @file
 */

#pragma once

#include <cstdint>

namespace voxedit {

enum class SelectType : uint8_t {
	Single,
	Same,
	LineVertical,
	LineHorizontal,
	Edge,

	Max
};

}
