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
	std::string qualifier;
	std::string type;
	std::string name;
	std::string comment;
	std::string value;
	bool byReference = false;
	DataType datatype = DataType::None;
	compute::BufferFlag flags = compute::BufferFlag::ReadWrite;
};

struct ReturnValue {
	std::string type;
};

struct Kernel {
	std::string name;
	std::vector<Parameter> parameters;
	int workDimension = 1;
	ReturnValue returnValue;
};

struct Struct {
	bool isEnum = false;
	std::string comment;
	std::string name;
	std::vector<Parameter> parameters;
};

}
