/**
 * @file
 */

#pragma once

#include "compute/Types.h"
#include "core/String.h"
#include <vector>

namespace computeshadertool {

enum class DataType {
	None,
	Image2D,
	Image3D,
	Sampler,

	Max
};

struct Parameter {
	core::String qualifier;
	core::String type;
	core::String name;
	core::String comment;
	core::String value;
	bool byReference = false;
	DataType datatype = DataType::None;
	compute::BufferFlag flags = compute::BufferFlag::ReadWrite;
};

struct ReturnValue {
	core::String type;
};

struct Kernel {
	core::String name;
	std::vector<Parameter> parameters;
	int workDimension = 1;
	ReturnValue returnValue;
};

struct Struct {
	bool isEnum = false;
	core::String comment;
	core::String name;
	std::vector<Parameter> parameters;
};

}
