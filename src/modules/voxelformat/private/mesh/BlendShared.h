/**
 * @file
 */

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace voxelformat {

struct DNAChunk {
	uint32_t identifier;	   // File-block identifier
	uint32_t length;		   // Total length of the data after the file-block-header
	uint64_t oldMemoryAddress; // Memory address the structure was located when written to disk
	uint32_t indexSDNA;		   // Index of the SDNA structure
	uint32_t count;			   // Number of structure located in this file-block
};

struct Type {
	int16_t size;
	core::String name;
};

struct Field {
	core::String type;
	core::String name;
	size_t arraySize[2]{1, 1};
	size_t size = 0;
	bool isArray() const {
		return name.last() == ']';
	}
	bool isPointer() const {
		return name.first() == '*';
	}
};

struct Structure {
	uint16_t type;
	core::String name;
	core::DynamicArray<Field> fields;
};

void calcSize(Field &field, const Type &type, bool is64Bit);

} // namespace voxelformat
