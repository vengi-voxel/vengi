#pragma once

namespace voxel {

template<typename Type>
Type lerp(const Type& v0, const Type& v1, const float x) {
	//Interpolate along X
	Type v0_1 = (v1 - v0) * x + v0;

	return v0_1;
}

template<typename Type>
Type bilerp(const Type& v00, const Type& v10, const Type& v01, const Type& v11, const float x, const float y) {
	// Linearly interpolate along x
	Type v00_10 = lerp(v00, v10, x);
	Type v01_11 = lerp(v01, v11, x);

	// And linearly interpolate the results along y
	Type v00_10__v01_11 = lerp(v00_10, v01_11, y);

	return v00_10__v01_11;
}

template<typename Type>
Type trilerp(const Type& v000, const Type& v100, const Type& v010, const Type& v110, const Type& v001, const Type& v101, const Type& v011, const Type& v111, const float x,
		const float y, const float z) {
	// Bilinearly interpolate along Y
	Type v000_v100__v010_v110 = bilerp(v000, v100, v010, v110, x, y);
	Type v001_v101__v011_v111 = bilerp(v001, v101, v011, v111, x, y);

	// And linearly interpolate the results along z
	Type v000_v100__v010_v110____v001_v101__v011_v111 = lerp(v000_v100__v010_v110, v001_v101__v011_v111, z);

	return v000_v100__v010_v110____v001_v101__v011_v111;
}

}
