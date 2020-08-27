/**
 * @file
 */

#pragma once

#include "voxelgenerator/LUAGenerator.h"

namespace voxedit {

class EditorLUAGenerator : public voxelgenerator::LUAGenerator {
protected:
	void initializeCustomState(lua_State* s) override;
};

}
