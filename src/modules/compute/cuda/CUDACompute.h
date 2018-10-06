/**
 * @file
 */

#pragma once

#include "CUDA.h"
//#include "CUDASymbol.h"
//#include "CUDAMapping.h"
#include "compute/Compute.h"
#include "core/Assert.h"
#include "core/Common.h"
#include <vector>

namespace compute {

namespace _priv {
struct Context {
	bool features[std::enum_value(compute::Feature::Max)] = { };
	inline bool supports(compute::Feature feature) const {
		return features[std::enum_value(feature)];
	}
};

extern Context _ctx;

}

}
