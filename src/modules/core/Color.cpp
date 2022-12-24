/**
 * @file
 */

#include "Color.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "math/Octree.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/ext/scalar_integer.hpp>

#include <SDL.h>
#include <stdio.h>

namespace core {

const glm::vec4 Color::Clear = glm::vec4(0.f, 0, 0, 0) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::White = glm::vec4(255.f, 255, 255, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Black = glm::vec4(0.f, 0, 0, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Lime = glm::vec4(109.f, 198, 2, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Pink = glm::vec4(248.f, 4, 62, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::LightBlue = glm::vec4(0.f, 153, 203, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::DarkBlue = glm::vec4(55.f, 116, 145, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Orange = glm::vec4(252.f, 167, 0, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Yellow = glm::vec4(255.f, 255, 0, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Sandy = glm::vec4(237.f, 232, 160, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::LightGray = glm::vec4(192.f, 192, 192, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Gray = glm::vec4(128.f, 128, 128, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::DarkGray = glm::vec4(84.f, 84, 84, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::LightRed = glm::vec4(255.f, 96, 96, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Red = glm::vec4(255.f, 0, 0, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::DarkRed = glm::vec4(128.f, 0, 0, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::LightGreen = glm::vec4(96.f, 255, 96, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Green = glm::vec4(0.f, 255, 0, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::DarkGreen = glm::vec4(0.f, 128, 0, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Blue = glm::vec4(0.f, 0, 255, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::SteelBlue = glm::vec4(35.f, 107, 142, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Olive = glm::vec4(128.f, 128, 0, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Purple = glm::vec4(128.f, 0, 128, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Cyan = glm::vec4(0.f, 255, 255, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::Brown = glm::vec4(107.f, 66, 38, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::LightBrown = glm::vec4(150.f, 107, 72, 255) / glm::vec4(Color::magnitudef);
const glm::vec4 Color::DarkBrown = glm::vec4(82.f, 43, 26, 255) / glm::vec4(Color::magnitudef);

const float Color::magnitudef = 255.0f;
const float Color::scaleFactor = 0.7f;

int Color::quantize(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors, ColorReductionType type) {
	if (inputBufColors <= maxTargetBufColors) {
		size_t n;
		for (n = 0; n < inputBufColors; ++n) {
			targetBuf[n] = inputBuf[n];
		}
		for (size_t i = n; i < maxTargetBufColors; ++i) {
			targetBuf[i] = RGBA(255, 255, 255, 255);
		}
		return (int)n;
	}
	switch (type) {
	case ColorReductionType::Wu:
		return quantizeWu(targetBuf, maxTargetBufColors, inputBuf, inputBufColors);
	case ColorReductionType::Octree:
		return quantizeOctree(targetBuf, maxTargetBufColors, inputBuf, inputBufColors);
	default:
		break;
	}
	return -1;
}

int Color::quantizeOctree(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors) {
	core_assert(glm::isPowerOfTwo(maxTargetBufColors));
	core_assert(maxTargetBufColors == 256);
	using BBox = math::AABB<uint8_t>;
	struct ColorNode {
		inline ColorNode(core::RGBA c) : color(c){};
		core::RGBA color;
		inline BBox aabb() const {
			return BBox(color.r, color.g, color.b, color.r + 1, color.g + 1, color.b + 1);
		}
	};
	const BBox aabb(0, 0, 0, 255, 255, 255);
	using Tree = math::Octree<ColorNode, uint8_t>;
	Tree octree(aabb, 32);
	for (size_t i = 0; i < inputBufColors; ++i) {
		octree.insert(inputBuf[i]);
	}
	size_t n = 0;
	constexpr glm::ivec3 dim(8);
	const int rmax = aabb.getWidthX() + 1 - dim.r;
	const int gmax = aabb.getWidthY() + 1 - dim.g;
	const int bmax = aabb.getWidthZ() + 1 - dim.b;
	for (int r = 0; r <= rmax; r += dim.r) {
		for (int g = 0; g <= gmax; g += dim.g) {
			for (int b = 0; b <= bmax; b += dim.b) {
				Tree::Contents contents;
				const BBox queryAABB(r, g, b, r + dim.r - 1, g + dim.g - 1, b + dim.b - 1);
				octree.query(queryAABB, contents);
				const int k = (int)contents.size();
				if (k == 0) {
					continue;
				}
				targetBuf[n++] = contents.front().color;
				if (n >= maxTargetBufColors) {
					return (int)n;
				}
			}
		}
	}
	for (size_t i = n; i < maxTargetBufColors; ++i) {
		targetBuf[i] = RGBA(0xFFFFFFFFU);
	}
	return (int)n;
}

int Color::quantizeWu(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors) {
	struct Box {
		RGBA min, max;
		core::Buffer<RGBA> pixels;
	};

	core::Buffer<RGBA> pixels;
	pixels.append(inputBuf, inputBufColors);

	// Initialize the set of boxes with the full color range
	core::DynamicArray<Box> boxes;
	boxes.emplace_back(Box{{0, 0, 0, 255}, {255, 255, 255, 255}, pixels});

	// Iterate until we reach the desired number of boxes
	while (boxes.size() < maxTargetBufColors) {
		// Find the box with the largest volume (i.e., the most pixels)
		int maxVolume = std::numeric_limits<int>::min();
		size_t maxVolumeIndex = 0;
		for (size_t i = 0; i < boxes.size(); ++i) {
			int volume = (boxes[i].max.r - boxes[i].min.r + 1) * (boxes[i].max.g - boxes[i].min.g + 1) *
						 (boxes[i].max.b - boxes[i].min.b + 1);
			if (volume > maxVolume) {
				maxVolume = volume;
				maxVolumeIndex = i;
			}
		}

		// Split the box with the largest volume into two boxes along its longest dimension
		const Box &box = boxes[maxVolumeIndex];
		const size_t pixelCount = box.pixels.size();
		if (pixelCount <= 0) {
			boxes.erase(maxVolumeIndex);
			continue;
		}

		int component = 0;
		int midpoint = 0;
		if (box.max.r - box.min.r > box.max.g - box.min.g && box.max.r - box.min.r > box.max.b - box.min.b) {
			component = 0;
			midpoint = (box.min.r + box.max.r) / 2;
		} else if (box.max.g - box.min.g > box.max.b - box.min.b) {
			component = 1;
			midpoint = (box.min.g + box.max.g) / 2;
		} else {
			component = 2;
			midpoint = (box.min.b + box.max.b) / 2;
		}

		Box box1, box2;
		box1.pixels.reserve(pixelCount / 2);
		box2.pixels.reserve(pixelCount / 2);
		switch (component) {
		case 0:
			box1.min = box.min;
			box1.max = RGBA(midpoint, box.max.g, box.max.b, 255);
			box2.min = RGBA(midpoint + 1, box.min.g, box.min.b, 255);
			box2.max = box.max;
			for (const RGBA &pixel : box.pixels) {
				if (pixel.r <= midpoint) {
					box1.pixels.push_back(pixel);
				} else {
					box2.pixels.push_back(pixel);
				}
			}
			break;
		case 1:
			box1.min = box.min;
			box1.max = RGBA(box.max.r, midpoint, box.max.b, 255);
			box2.min = RGBA(box.min.r, midpoint + 1, box.min.b, 255);
			box2.max = box.max;
			for (const RGBA &pixel : box.pixels) {
				if (pixel.g <= midpoint) {
					box1.pixels.push_back(pixel);
				} else {
					box2.pixels.push_back(pixel);
				}
			}
			break;
		case 2:
			box1.min = box.min;
			box1.max = RGBA(box.max.r, box.max.g, midpoint);
			box2.min = RGBA(box.min.r, box.min.g, midpoint + 1);
			box2.max = box.max;
			for (const RGBA &pixel : box.pixels) {
				if (pixel.b <= midpoint) {
					box1.pixels.push_back(pixel);
				} else {
					box2.pixels.push_back(pixel);
				}
			}
			break;
		}

		// Replace the original box with the two split boxes
		boxes.erase(maxVolumeIndex);
		boxes.emplace_back(core::move(box1));
		boxes.emplace_back(core::move(box2));
	}

	size_t n = 0;
	for (const Box& box : boxes) {
		if (box.pixels.empty()) {
			continue;
		}
		RGBA average(0, 0, 0, 255);
		for (const RGBA& pixel : box.pixels) {
			average.r += pixel.r;
			average.g += pixel.g;
			average.b += pixel.b;
		}
		average.r /= box.pixels.size();
		average.g /= box.pixels.size();
		average.b /= box.pixels.size();
		targetBuf[n++] = average;
	}

	for (size_t i = n; i < maxTargetBufColors; ++i) {
		targetBuf[i] = RGBA(0xFFFFFFFFU);
	}
	return (int)n;
}

glm::vec4 Color::fromRGBA(const RGBA rgba) {
	return fromRGBA(rgba.r, rgba.g, rgba.b, rgba.a);
}

glm::vec4 Color::fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	return glm::vec4(r, g, b, a) / Color::magnitudef;
}

glm::vec4 Color::fromHSB(const float hue, const float saturation, const float brightness, const float alpha) {
	if (0.00001f > brightness) {
		return glm::vec4(0.f, 0.f, 0.f, alpha);
	}
	if (0.00001f > saturation) {
		return glm::vec4(brightness, brightness, brightness, alpha);
	}
	glm::vec4 color(0.0f, 0.0f, 0.0f, alpha);
	const float h = (hue - (float)SDL_floor(hue)) * 6.f;
	const float f = h - (float)SDL_floor(h);
	const float p = brightness * (1.f - saturation);
	const float q = brightness * (1.f - saturation * f);
	const float t = brightness * (1.f - (saturation * (1.f - f)));
	switch (static_cast<int>(h)) {
	case 0:
		color.r = brightness;
		color.g = t;
		color.b = p;
		break;
	case 1:
		color.r = q;
		color.g = brightness;
		color.b = p;
		break;
	case 2:
		color.r = p;
		color.g = brightness;
		color.b = t;
		break;
	case 3:
		color.r = p;
		color.g = q;
		color.b = brightness;
		break;
	case 4:
		color.r = t;
		color.g = p;
		color.b = brightness;
		break;
	case 5:
		color.r = brightness;
		color.g = p;
		color.b = q;
		break;
	}
	return color;
}

core::String Color::toHex(const RGBA rgba, bool hashPrefix) {
	core::String hex;
	if (hashPrefix) {
		hex.append("#");
	}
	hex.append(core::string::format("%02x%02x%02x%02x", rgba.r, rgba.g, rgba.b, rgba.a));
	return hex;
}

glm::vec4 Color::fromHex(const char *hex) {
	uint32_t r = 0x00;
	uint32_t g = 0x00;
	uint32_t b = 0x00;
	uint32_t a = 0xff;
	if (0 == SDL_strncmp(hex, "0x", 2) || 0 == SDL_strncmp(hex, "0X", 2)) {
		hex += 2;
	} else if (hex[0] == '#') {
		hex += 1;
	}
	if (sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a) == 3) {
		a = 0xff;
	}
	return fromRGBA(r, g, b, a);
}

float Color::getDistance(const glm::vec4 &color, float hue, float saturation, float brightness) {
	float chue;
	float csaturation;
	float cbrightness;
	core::Color::getHSB(color, chue, csaturation, cbrightness);

	const float weightHue = 0.8f;
	const float weightSaturation = 0.1f;
	const float weightValue = 0.1f;

	const float dH = chue - hue;
	const float dS = csaturation - saturation;
	const float dV = cbrightness - brightness;
	const float val = weightHue * (float)glm::pow(dH, 2) + weightValue * (float)glm::pow(dV, 2) +
					  weightSaturation * (float)glm::pow(dS, 2);
	return val;
}

core::String Color::print(RGBA rgba, bool colorAsHex) {
	String buf = "\033[0m";
	if (colorAsHex) {
		buf = toHex(rgba);
		buf.append(" ");
	}
	if (rgba.a != 0) {
		buf.append("\033[38;2;");
		buf.append(rgba.r).append(";");
		buf.append(rgba.g).append(";");
		buf.append(rgba.b).append("m");
	}
	buf.append("\033[48;2;");
	buf.append(rgba.r).append(";");
	buf.append(rgba.g).append(";");
	buf.append(rgba.b).append("m");
	buf.append(u8"\u2587");
	buf.append("\033[0m");
	return buf;
}

float Color::getDistance(RGBA rgba, RGBA rgba2) {
	const glm::vec4 &color = core::Color::fromRGBA(rgba);
	float hue;
	float saturation;
	float brightness;
	core::Color::getHSB(color, hue, saturation, brightness);
	return core::Color::getDistance(rgba2, hue, saturation, brightness);
}

float Color::getDistance(RGBA rgba, float hue, float saturation, float brightness) {
	const glm::vec4 &color = core::Color::fromRGBA(rgba);
	return getDistance(color, hue, saturation, brightness);
}

core::RGBA Color::flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t f) {
	if (f <= 1u) {
		return core::RGBA(r, g, b, a);
	}
	return core::RGBA(r / f * f, g / f * f, b / f * f, a);
}

RGBA Color::getRGBA(const glm::vec4 &color) {
	return RGBA{(uint8_t)(color.r * magnitude), (uint8_t)(color.g * magnitude), (uint8_t)(color.b * magnitude),
				(uint8_t)(color.a * magnitude)};
}

void Color::getHSB(const glm::vec4 &color, float &chue, float &csaturation, float &cbrightness) {
	cbrightness = brightness(color);
	const float minBrightness = core_min(color.r, core_min(color.g, color.b));
	if (SDL_fabs(cbrightness - minBrightness) < 0.00001f) {
		chue = 0.f;
		csaturation = 0.f;
		return;
	}
	const float r = (cbrightness - color.r) / (cbrightness - minBrightness);
	const float g = (cbrightness - color.g) / (cbrightness - minBrightness);
	const float b = (cbrightness - color.b) / (cbrightness - minBrightness);
	if (SDL_fabs(color.r - cbrightness) < 0.00001f) {
		chue = b - g;
	} else if (SDL_fabs(color.g - cbrightness) < 0.00001f) {
		chue = 2.f + r - b;
	} else {
		chue = 4.f + g - r;
	}
	chue /= 6.f;
	if (chue < 0.f) {
		chue += 1.f;
	}
	csaturation = (cbrightness - minBrightness) / cbrightness;
}

glm::vec4 Color::alpha(const glm::vec4 &c, float alpha) {
	return glm::vec4(c.r, c.g, c.b, alpha);
}

RGBA Color::alpha(const RGBA rgba, uint8_t alpha) {
	return RGBA(rgba.r, rgba.g, rgba.b, alpha);
}

float Color::brightness(const glm::vec4 &color) {
	return core_max(color.r, core_max(color.g, color.b));
}

float Color::intensity(const glm::vec4 &color) {
	return (color.r + color.g + color.b) / 3.f;
}

glm::vec4 Color::gray(const glm::vec4 &color) {
	const float gray = (0.21f * color.r + 0.72f * color.g + 0.07f * color.b) / 3.0f;
	return glm::vec4(gray, gray, gray, color.a);
}

glm::vec3 Color::gray(const glm::vec3 &color) {
	const float gray = (0.21f * color.r + 0.72f * color.g + 0.07f * color.b) / 3.0f;
	return glm::vec3(gray, gray, gray);
}

glm::vec4 Color::darker(const glm::vec4 &color, float f) {
	f = (float)SDL_pow(scaleFactor, f);
	return glm::vec4(glm::clamp(glm::vec3(color) * f, 0.0f, 1.0f), color.a);
}

glm::vec4 Color::brighter(const glm::vec4 &color, float f) {
	static float min = 21.f / magnitude;
	glm::vec3 result = glm::vec3(color);
	f = (float)SDL_pow(scaleFactor, f);
	if (glm::all(glm::epsilonEqual(glm::zero<glm::vec3>(), result, 0.00001f))) {
		return glm::vec4(min / f, min / f, min / f, color.a);
	}
	if (result.r > 0.f && result.r < min) {
		result.r = min;
	}
	if (result.g > 0.f && result.g < min) {
		result.g = min;
	}
	if (result.b > 0.f && result.b < min) {
		result.b = min;
	}
	return glm::vec4(glm::clamp(result / f, 0.f, 1.f), color.a);
}

} // namespace core
