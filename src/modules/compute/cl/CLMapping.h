/**
 * @file
 */

#pragma once

#include "compute/Types.h"
#include "core/Array.h"
#include "CL.h"

namespace compute {

namespace _priv {

static size_t TextureFormatComponents[] {
	4,
	3,
	4,
	4,
	2
};
static_assert(std::enum_value(TextureFormat::Max) == lengthof(TextureFormatComponents), "Array sizes don't match Max");

static cl_channel_order TextureFormats[] {
	CL_RGBA,
	CL_RGB,
	CL_BGRA,
	CL_ARGB,
	CL_RG
};
static_assert(std::enum_value(TextureFormat::Max) == lengthof(TextureFormats), "Array sizes don't match Max");

static cl_channel_type TextureDataFormats[] {
	CL_SNORM_INT8,
	CL_SNORM_INT16,
	CL_UNORM_INT8,
	CL_UNORM_INT16,
	CL_UNORM_SHORT_565,
	CL_UNORM_SHORT_555,
	CL_UNORM_INT_101010,
	CL_SIGNED_INT8,
	CL_SIGNED_INT16,
	CL_SIGNED_INT32,
	CL_UNSIGNED_INT8,
	CL_UNSIGNED_INT16,
	CL_UNSIGNED_INT32,
	CL_HALF_FLOAT,
	CL_FLOAT,
};
static_assert(std::enum_value(TextureDataFormat::Max) == lengthof(TextureDataFormats), "Array sizes don't match Max");

static size_t TextureDataFormatSizes[] {
	1,
	2,
	1,
	2,
	2,
	2,
	4,
	1,
	2,
	4,
	1,
	2,
	4,
	2,
	4
};
static_assert(std::enum_value(TextureDataFormat::Max) == lengthof(TextureDataFormatSizes), "Array sizes don't match Max");

static cl_filter_mode TextureFilters[] {
	CL_FILTER_NEAREST,
	CL_FILTER_LINEAR
};
static_assert(std::enum_value(TextureFilter::Max) == lengthof(TextureFilters), "Array sizes don't match Max");

static cl_addressing_mode TextureWraps[] {
	CL_ADDRESS_NONE,
	CL_ADDRESS_CLAMP_TO_EDGE,
	CL_ADDRESS_CLAMP,
	CL_ADDRESS_REPEAT,
	CL_ADDRESS_MIRRORED_REPEAT
};
static_assert(std::enum_value(TextureWrap::Max) == lengthof(TextureWraps), "Array sizes don't match Max");

}

}
