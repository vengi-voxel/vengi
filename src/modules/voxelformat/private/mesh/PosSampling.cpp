/**
 * @file
 */

#include "PosSampling.h"
#include "color/ColorUtil.h"

namespace voxelformat {

uint8_t PosSampling::getNormal() const {
	if (entries[1].area == 0) {
		return entries[0].normal;
	}
	uint8_t normal = 0;
	uint32_t area = 0;
	for (const PosSamplingEntry &pe : entries) {
		if (pe.area > area) {
			area = pe.area;
			normal = pe.normal;
		}
	}
	return normal;
}

MeshMaterialIndex PosSampling::getMaterialIndex() const {
	if (entries[1].area == 0) {
		return entries[0].materialIdx;
	}
	uint32_t area = 0;
	MeshMaterialIndex materialIdx = -1;
	for (const PosSamplingEntry &pe : entries) {
		if (pe.area > area) {
			area = pe.area;
			materialIdx = pe.materialIdx;
		}
	}
	return materialIdx;

}

bool PosSampling::add(uint32_t area, color::RGBA color, uint8_t normal, MeshMaterialIndex materialIdx) {
	// TODO: VOXELFORMAT: why?
	if (entries[0].color == color) {
		return false;
	}
	if (area == 0) {
		// nothing to contribute
		return false;
	}
#if 0
	for (int i = 0; i < MaxTriangleColorContributions; ++i) {
		// same values only increase the area of contribution and thus the weighting influence for this color
		if (entries[i].area > 0 && entries[i].color == color && entries[i].normal == normal) {
			entries[i].area += area;
			return true;
		}
	}
#endif

#if 1
	for (int i = 0; i < MaxTriangleColorContributions; ++i) {
		// free slot
		if (entries[i].area == 0) {
			entries[i].area = area;
			entries[i].color = color;
			entries[i].normal = normal;
			entries[i].materialIdx = materialIdx;
			return true;
		}
	}
#else
	int smallestArea = 0xffffffff;
	int index = 0;
	for (int i = 0; i < MaxTriangleColorContributions; ++i) {
		// free slot
		if (entries[i].area == 0) {
			entries[i].area = area;
			entries[i].color = color;
			entries[i].normal = normal;
			entries[i].materialIdx = materialIdx;
			return true;
		}
		if (smallestArea > entries[i].area) {
			smallestArea = entries[i].area;
			index = i;
		}
	}
	// check if this contribution should have a higher impact
	if (entries[index].area < area) {
		entries[index].area = area;
		entries[index].color = color;
		entries[index].normal = normal;
		entries[index].materialIdx = materialIdx;
		return true;
	}
#endif

	return false;
}

color::RGBA PosSampling::getColor(uint8_t flattenFactor, bool weightedAverage) const {
	if (entries[1].area == 0) {
		return color::flattenRGB(entries[0].color.r, entries[0].color.g, entries[0].color.b, entries[0].color.a,
									   flattenFactor);
	}
	if (weightedAverage) {
		uint32_t sumArea = 0;
		for (const PosSamplingEntry &pe : entries) {
			sumArea += pe.area;
		}
		color::RGBA color(0, 0, 0, 255);
		if (sumArea == 0) {
			return color;
		}
		for (const PosSamplingEntry &pe : entries) {
			if (pe.area == 0) {
				break;
			}
			color = color::RGBA::mix(color, pe.color, (float)pe.area / (float)sumArea);
		}
		return color::flattenRGB(color.r, color.g, color.b, color.a, flattenFactor);
	}
	color::RGBA color(0, 0, 0, AlphaThreshold);
	uint32_t area = 0;
	for (const PosSamplingEntry &pe : entries) {
		if (pe.area == 0) {
			break;
		}
		if (pe.area > area) {
			area = pe.area;
			color = pe.color;
		}
	}
	return color::flattenRGB(color.r, color.g, color.b, color.a, flattenFactor);
}

} // namespace voxelformat
