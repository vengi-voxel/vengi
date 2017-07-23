#pragma once

#include "core/Common.h"

namespace compute {

enum class BufferFlag {
	None = 0,
	// default
	ReadWrite = 1,
	WriteOnly = 2,
	ReadOnly = 4,
	UseHostPointer = 8,
	AllocHostPointer = 16,
	CopyHostPointer = 32,

	Max = 7
};
CORE_ENUM_BIT_OPERATIONS(BufferFlag)

}
