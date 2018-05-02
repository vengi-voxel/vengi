/**
 * @file
 */

#pragma once

#include "Select.h"

namespace voxedit {
namespace selections {

/**
 * @brief Just select the voxel you clicked
 */
class Single : public Select {
public:
	SelectionSingleton(Single)
};

}
}
