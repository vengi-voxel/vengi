/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"

namespace io {
class WriteStream;
}

namespace core {
class Var;
typedef core::SharedPtr<Var> VarPtr;
}

namespace voxelformat {

class FormatConfig {
public:
	static bool init();
	static void writeConfigJson(io::WriteStream &stream, const core::VarPtr &var);
};

} // namespace voxelformat
