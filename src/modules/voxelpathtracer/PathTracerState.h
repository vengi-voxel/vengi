/**
 * @file
 */

#pragma once

#include "core/ArrayLength.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "voxelrender/RenderUtil.h"
#include <glm/vec3.hpp>
#include <stdint.h>

namespace voxelpathtracer {

enum PathTracerBackground {
	PathTracerBackgroundSky = 0,
	PathTracerBackgroundSolid = 1,
	PathTracerBackgroundTransparent = 2
};

enum class PathTracerQuality : uint8_t { Default = 0, Preview = 1, High = 2, Max };
static constexpr const char *PathTracerQualityArgs[] = {"default", "preview", "high"};
static_assert(lengthof(PathTracerQualityArgs) == (int)PathTracerQuality::Max,
			  "PathTracerQualityArgs size must match PathTracerQuality");

struct PathTracerParams {
	int samples = 256;
	int bounces = 4;
	int samplesThisPass = 1;
	int resolution = 512;
	int camera = 0;
	float exposure = 0.0f;
	int toneMapping = voxelrender::ToneMappingAces;
	float contrast = 1.0f;
	float saturation = 1.0f;
	float vignette = 0.25f;
	float aperture = 0.0f;
	float focusDistance = 40.0f;
	float sunAzimuth = 135.0f;
	float sunElevation = 35.0f;
	float sunIntensity = 3.0f;
	float sunSoftness = 1.5f;
	glm::vec3 sunColor{1.0f, 0.945f, 0.863f};
	glm::vec3 skyTop{1.0f};
	glm::vec3 skyHorizon{1.0f};
	float skyIntensity = 1.0f;
	bool floorEnabled = true;
	bool depthOfField = false;
	glm::vec3 floorColor{80.0f / 255.0f};
	float floorY = 0.0f;
	int background = PathTracerBackgroundSky;
	glm::vec3 backgroundColor{27.0f / 255.0f, 29.0f / 255.0f, 34.0f / 255.0f};
	float fogDensity = 0.0f;
	glm::vec3 fogColor{184.0f / 255.0f, 198.0f / 255.0f, 217.0f / 255.0f};
	float voxelEdge = 0.0f;
	float bloom = 0.0f;
	float atmosphere = 0.0f;
	float renderScale = 1.0f;
};

/** @brief Apply a quality preset. Default replaces all params; Preview/High only adjust quality fields. */
void applyPathTracerQuality(PathTracerParams &params, PathTracerQuality quality);

/** @brief Parse a CLI/config quality name; returns false when unknown. */
bool pathTracerQualityFromName(const core::String &name, PathTracerQuality &out);

struct PathTracerState {
	PathTracerParams params;
	core::DynamicArray<core::String> cameraNames;
	bool started = false;
	bool volumeTruncated = false;
	int currentSample = 0;
};

} // namespace voxelpathtracer
