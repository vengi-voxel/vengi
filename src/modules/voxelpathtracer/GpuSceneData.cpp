/**
 * @file
 */
#include "GpuSceneData.h"
#include "PathTracerState.h"
#include "color/ColorUtil.h"
#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "palette/Material.h"
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>

namespace voxelpathtracer {

namespace {
// Memory budget for the coarse indirection grid (sized as 32-bit cells).
constexpr size_t MaxIndirectionBytes = 512ull * 1024ull * 1024ull;
} // namespace

void applyPathTracerQuality(PathTracerParams &params, PathTracerQuality quality) {
	switch (quality) {
	case PathTracerQuality::Default:
		params = PathTracerParams();
		break;
	case PathTracerQuality::Preview:
		params.samples = 16;
		params.bounces = 2;
		params.samplesThisPass = 1;
		break;
	case PathTracerQuality::High:
		params.samples = 1024;
		params.bounces = 8;
		params.samplesThisPass = 1;
		params.voxelEdge = 0.4f;
		params.bloom = 0.35f;
		break;
	case PathTracerQuality::Max:
		break;
	}
}

bool pathTracerQualityFromName(const core::String &name, PathTracerQuality &out) {
	for (int i = 0; i < (int)PathTracerQuality::Max; ++i) {
		if (core::string::iequals(name, PathTracerQualityArgs[i])) {
			out = (PathTracerQuality)i;
			return true;
		}
	}
	return false;
}

int padToBrick(int v) {
	return ((v + BrickSize - 1) / BrickSize) * BrickSize;
}

glm::ivec3 clampVolumeSize(glm::ivec3 size, int maxAxis, bool &truncated) {
	truncated = false;
	if (size.x > maxAxis || size.y > maxAxis || size.z > maxAxis) {
		truncated = true;
		size = glm::min(size, glm::ivec3(maxAxis));
	}
	size.x = padToBrick(size.x);
	size.y = padToBrick(size.y);
	size.z = padToBrick(size.z);
	size = glm::max(size, glm::ivec3(BrickSize));

	glm::ivec3 coarse(size.x / BrickSize, size.y / BrickSize, size.z / BrickSize);
	// Indirection is a 3D texture sized in bricks; maxAxis is @c Limit::Max3DTextureSize.
	const int maxCoarseAxis = core_max(1, maxAxis);
	if (coarse.x > maxCoarseAxis || coarse.y > maxCoarseAxis || coarse.z > maxCoarseAxis) {
		truncated = true;
		coarse = glm::min(coarse, glm::ivec3(maxCoarseAxis));
	}

	size_t coarseCount = (size_t)coarse.x * (size_t)coarse.y * (size_t)coarse.z;
	const size_t maxCells = MaxIndirectionBytes / sizeof(uint32_t);
	if (coarseCount > maxCells) {
		truncated = true;
		const float scale = glm::pow((float)maxCells / (float)coarseCount, 1.0f / 3.0f);
		coarse.x = core_max(1, (int)((float)coarse.x * scale));
		coarse.y = core_max(1, (int)((float)coarse.y * scale));
		coarse.z = core_max(1, (int)((float)coarse.z * scale));
		coarse = glm::min(coarse, glm::ivec3(maxCoarseAxis));
		coarseCount = (size_t)coarse.x * (size_t)coarse.y * (size_t)coarse.z;
		while (coarseCount > maxCells) {
			if (coarse.x >= coarse.y && coarse.x >= coarse.z && coarse.x > 1) {
				--coarse.x;
			} else if (coarse.y >= coarse.z && coarse.y > 1) {
				--coarse.y;
			} else if (coarse.z > 1) {
				--coarse.z;
			} else {
				break;
			}
			coarseCount = (size_t)coarse.x * (size_t)coarse.y * (size_t)coarse.z;
		}
	}

	return glm::ivec3(coarse.x * BrickSize, coarse.y * BrickSize, coarse.z * BrickSize);
}

// Jarzynski & Olano, "Hash Functions for GPU Rendering" (PCG RXS-M-XS).
// https://jcgt.org/published/0009/03/02/
// Keep identical to pcgHash in voxelrender/shaders/_random.glsl.
uint32_t sampleSeedHash(uint32_t sampleIndex) {
	const uint32_t state = sampleIndex * 747796405u + 2891336453u;
	const uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

// Sobol sequence, first two dimensions in Gray-code order.
// https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sobol_Samplers
glm::uvec2 cameraSobolPoint(uint32_t sampleIndex) {
	uint32_t gray = sampleIndex ^ (sampleIndex >> 1u);
	glm::uvec2 point(0u);
	uint32_t direction = 0x80000000u;
	for (uint32_t bit = 0u; gray != 0u; ++bit, gray >>= 1u) {
		if ((gray & 1u) != 0u) {
			point.x ^= 0x80000000u >> bit;
			point.y ^= direction;
		}
		direction ^= direction >> 1u;
	}
	return point;
}

void buildEmptySpaceHierarchy(uint8_t *coarse, const glm::ivec3 &size) {
	for (int level = 1; level <= 3; ++level) {
		const int width = 1 << level;
		for (int z = 0; z < size.z; z += width) {
			for (int y = 0; y < size.y; y += width) {
				for (int x = 0; x < size.x; x += width) {
					const glm::ivec3 end = glm::min(glm::ivec3(x, y, z) + width, size);
					bool occupied = false;
					for (int cz = z; cz < end.z && !occupied; ++cz) {
						for (int cy = y; cy < end.y && !occupied; ++cy) {
							for (int cx = x; cx < end.x; ++cx) {
								if (coarse[((size_t)cz * size.y + cy) * size.x + cx] == 255u) {
									occupied = true;
									break;
								}
							}
						}
					}
					if (occupied) {
						continue;
					}
					for (int cz = z; cz < end.z; ++cz) {
						for (int cy = y; cy < end.y; ++cy) {
							for (int cx = x; cx < end.x; ++cx) {
								coarse[((size_t)cz * size.y + cy) * size.x + cx] = (uint8_t)level;
							}
						}
					}
				}
			}
		}
	}
}

// Walker's alias method with Vose's table construction, weighted by emitter luminance.
// https://pbr-book.org/4ed/Sampling_Algorithms/The_Alias_Method
float buildEmitterAliasTable(GpuEmitLight *lights, int count) {
	float total = 0.0f;
	for (int i = 0; i < count; ++i) {
		total += glm::dot(glm::vec3(lights[i].radianceR, lights[i].radianceG, lights[i].radianceB),
						  glm::vec3(0.2126f, 0.7152f, 0.0722f));
	}
	core::DynamicArray<float> scaled(count);
	core::DynamicArray<int> small, large;
	for (int i = 0; i < count; ++i) {
		const float luminance = glm::dot(glm::vec3(lights[i].radianceR, lights[i].radianceG, lights[i].radianceB),
										 glm::vec3(0.2126f, 0.7152f, 0.0722f));
		scaled[i] = total > 0.0f ? luminance * (float)count / total : 1.0f;
		lights[i].aliasThreshold = 1.0f;
		lights[i].aliasIndex = (float)i;
		if (scaled[i] < 1.0f) {
			small.push_back(i);
		} else {
			large.push_back(i);
		}
	}
	while (!small.empty() && !large.empty()) {
		const int low = small.back();
		small.pop();
		const int high = large.back();
		large.pop();
		lights[low].aliasThreshold = scaled[low];
		lights[low].aliasIndex = (float)high;
		scaled[high] += scaled[low] - 1.0f;
		if (scaled[high] < 1.0f) {
			small.push_back(high);
		} else {
			large.push_back(high);
		}
	}
	return total;
}

// Environment importance sampling for the custom sky gradient.
// Integrate over bands in y = cos(theta), where dOmega = 2*pi*dy.
// https://pbr-book.org/4ed/Light_Sources/Infinite_Area_Lights
float buildSkyDistribution(const glm::vec3 &top, const glm::vec3 &horizon, const glm::vec3 &nadir, glm::vec4 *bands) {
	const glm::vec3 weights(0.2126f, 0.7152f, 0.0722f);
	const float topLuma = glm::dot(top, weights);
	const float horizonLuma = glm::dot(horizon, weights);
	const float nadirLuma = glm::dot(nadir, weights);
	float integral = 0.0f;
	for (int i = 0; i < 16; ++i) {
		const float y0 = -1.0f + (float)i / 8.0f;
		const float y1 = y0 + 0.125f;
		const float a = glm::min(glm::abs(y0), glm::abs(y1));
		const float b = glm::max(glm::abs(y0), glm::abs(y1));
		const float gradientIntegral = b * b * b - 0.5f * b * b * b * b - a * a * a + 0.5f * a * a * a * a;
		const float endLuma = y0 >= 0.0f ? topLuma : nadirLuma;
		const float mass = glm::max(0.0f, horizonLuma * 0.125f + (endLuma - horizonLuma) * gradientIntegral);
		integral += mass;
		bands[i] = glm::vec4(integral, mass, 0.0f, 0.0f);
	}
	if (integral <= 0.0f) {
		return 0.0f;
	}
	for (int i = 0; i < 16; ++i) {
		bands[i].x /= integral;
		bands[i].y /= integral * (2.0f * 3.14159265358979323846f * 0.125f);
	}
	bands[15].x = 1.0f;
	const float maximum = glm::max(topLuma, glm::max(horizonLuma, nadirLuma));
	const float minimum = glm::min(topLuma, glm::min(horizonLuma, nadirLuma));
	// A nearly uniform environment is already sampled well by the BSDF.
	return maximum - minimum > maximum * 0.01f ? integral * (2.0f * 3.14159265358979323846f) : 0.0f;
}

GpuMaterial packGpuMaterial(const palette::Material &mat, uint8_t alpha, bool voxelTransparent) {
	GpuMaterial gpu;
	core_memset(&gpu, 0, sizeof(gpu));
	gpu.type = (float)(int)mat.type;
	gpu.roughness = mat.roughness;
	gpu.metallic = mat.metal;
	gpu.emission = mat.emit;
	gpu.ior = mat.indexOfRefraction;
	gpu.transparency = alpha < 255 ? (1.0f - (float)alpha / 255.0f) : (voxelTransparent ? 1.0f : 0.0f);
	gpu.specular = mat.specular;
	gpu.attenuation = mat.attenuation;
	gpu.flux = mat.flux;
	gpu.density = mat.density;
	gpu.phase = mat.phase;
	gpu.media = mat.media;
	gpu.ldr = mat.lowDynamicRange;
	gpu.sp = mat.sp;
	gpu.mediatype = mat.media;
	// Used when remapping bricks to set VoxelFlagTransmits on each voxel payload.
	gpu.transmits =
		gpu.transparency > 0.0f || mat.type == palette::MaterialType::Glass || mat.type == palette::MaterialType::Media
			? 1.0f
			: 0.0f;
	return gpu;
}

GpuEmitLight packGpuEmitLight(const glm::ivec3 &cell, const glm::vec3 &srgb, const GpuMaterial &material) {
	const float emission = glm::max(material.emission, 0.0f);
	const float flux = glm::clamp(material.flux, 0.0f, 8.0f);
	const float scale = glm::mix(emission * glm::exp2(flux), emission, glm::clamp(material.ldr, 0.0f, 1.0f));
	const glm::vec3 radiance = color::srgbToLinear(srgb) * scale;
	GpuEmitLight light{};
	light.cellx = (float)cell.x;
	light.celly = (float)cell.y;
	light.cellz = (float)cell.z;
	light.radianceR = radiance.r;
	light.radianceG = radiance.g;
	light.radianceB = radiance.b;
	return light;
}

} // namespace voxelpathtracer
