#pragma once

#include "Types.h"

namespace video {

/**
 * @brief Maps data types to GL enums
 */
template<class DATATYPE>
constexpr inline DataType mapType() {
	if (std::is_floating_point<DATATYPE>()) {
		return DataType::Float;
	}

	if (sizeof(DATATYPE) == 1u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedByte;
		}
		return DataType::Byte;
	}

	if (sizeof(DATATYPE) == 2u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedShort;
		}
		return DataType::Short;
	}

	if (sizeof(DATATYPE) == 4u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedInt;
		}
		return DataType::Int;
	}

	return DataType::Max;
}

}

#include "gl/GLRenderer.h"
