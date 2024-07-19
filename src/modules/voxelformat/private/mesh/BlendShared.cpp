
#include "BlendShared.h"
#include "core/StringUtil.h"

namespace voxelformat {

void calcSize(Field &field, const Type &type, bool is64Bit) {
	core_assert(!field.name.empty());
	if (field.isArray()) {
		const int first = field.name.find_first_of("[");
		const int second = field.name.find_last_of("[");
		field.arraySize[0] = core::string::toInt(field.name.substr(first + 1));
		if (first != second) {
			field.arraySize[1] = core::string::toInt(field.name.substr(second + 1));
		}
	} else {
		field.arraySize[0] = field.arraySize[1] = 1;
	}
	if (field.isPointer()) {
		size_t ptrSize = is64Bit ? 8 : 4;
		field.size = ptrSize * field.arraySize[0] * field.arraySize[1];
	} else {
		field.size = type.size * field.arraySize[0] * field.arraySize[1];
	}
}

} // namespace voxelformat
