/**
 * @file
 */

#pragma once

#include "cl/CLTypes.h"
#include "core/Common.h"

namespace compute {

enum class TextureType {
	Texture1D,
	Texture2D,
	Texture3D,

	Max
};

enum class TextureDataFormat {
	SNORM_INT8,
	SNORM_INT16,
	UNORM_INT8,
	UNORM_INT16,
	UNORM_SHORT_565,
	UNORM_SHORT_555,
	UNORM_INT_101010,
	SIGNED_INT8,
	SIGNED_INT16,
	SIGNED_INT32,
	UNSIGNED_INT8,
	UNSIGNED_INT16,
	UNSIGNED_INT32,
	HALF_FLOAT,
	FLOAT,

	Max
};

enum class TextureFormat {
	RGBA,
	RGB,
	BGRA,
	ARGB,
	RG,
	R,

	Max
};

enum class TextureWrap {
	None,
	ClampToEdge,
	ClampToBorder,
	Repeat,
	MirroredRepeat,

	Max
};

enum class TextureFilter {
	Linear,
	Nearest,

	Max
};

/**
 * @brief Compute shader buffer flags
 */
enum class BufferFlag {
	None = 0,
	/** default */
	ReadWrite = 1,
	WriteOnly = 2,
	ReadOnly = 4,
	/**
	 * Use this when a buffer is already allocated as page-aligned with
	 * Shader::bufferAlloc() instead of @c malloc/new and a size that is a multiple of 64 bytes
	 * Also make sure to use Shader::bufferFree() instead of @c free/delete[]
	 */
	UseHostPointer = 8,
	/**
	 * when the data will be generated on the device but may be read back on the host.
	 * In this case leverage this flag to create the data
	 */
	AllocHostPointer = 16,
	CopyHostPointer = 32,

	Max = 7
};
CORE_ENUM_BIT_OPERATIONS(BufferFlag)

}
