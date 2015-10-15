#pragma once

#include "core/Log.h"
#include <sstream>

namespace noise {

template<class NOISE>
inline void printNoise(NOISE& noise, float width, float height, float depth) {
	for (float y = 0; y < 32; ++y) {
		Log::info("y: %f", y);
		Log::info("------------");
		for (float z = depth-1; z >= 0; --z) {
			Log::info("%.3f: ", z);
			std::stringstream sb;
			for (float x = 0; x < width; ++x) {
				sb << noise.get(x, y, z, 256.0) << " ";
			}
			Log::info("%s", sb.str().c_str());
		}
		Log::info("------------");
	}
}

}
