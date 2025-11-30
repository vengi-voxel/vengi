/**
 * @file
 */

#include "Quantize.h"
#include "color/ColorUtil.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "core/Pair.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "math/AABB.h"
#include "math/Octree.h"
#include <glm/common.hpp>
#include <glm/ext/scalar_integer.hpp>
#include <random>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/type_aligned.hpp>

namespace color {

static constexpr const char *ColorReductionAlgorithmStr[]{"Octree", "Wu", "MedianCut", "KMeans", "NeuQuant"};
static_assert((int)ColorReductionType::Max == lengthof(ColorReductionAlgorithmStr),
			  "Array size doesn't match with enum");

const char *toColorReductionTypeString(ColorReductionType type) {
	return ColorReductionAlgorithmStr[(int)type];
}

ColorReductionType toColorReductionType(const char *str) {
	for (int i = 0; i < lengthof(ColorReductionAlgorithmStr); ++i) {
		if (!SDL_strcasecmp(str, ColorReductionAlgorithmStr[i])) {
			return (ColorReductionType)i;
		}
	}
	Log::warn("Could not find a color reduction algorithm for '%s'", str);
	return ColorReductionType::Max;
}

struct ColorBox {
	RGBA min, max;
	core::Buffer<RGBA> pixels;

	ColorBox() = default;
	ColorBox(const RGBA &_min, const RGBA &_max, core::Buffer<RGBA> &&_pixels) : min(_min), max(_max), pixels(_pixels) {
	}
};

static int medianCutFindMedian(const core::Buffer<RGBA> &colors, int axis) {
	core::Buffer<int> values;
	values.reserve(colors.size());
	for (const color::RGBA &color : colors) {
		if (axis == 0) {
			values.push_back(color.r);
		} else if (axis == 1) {
			values.push_back(color.g);
		} else {
			values.push_back(color.b);
		}
	}
	core::sort(values.begin(), values.end(), core::Less<int>());
	return values[values.size() / 2];
}

static core::Pair<ColorBox, ColorBox> medianCutSplitBox(const ColorBox &box) {
	int longestAxis = 0;
	if ((box.max.g - box.min.g) > (box.max.r - box.min.r)) {
		longestAxis = 1;
	}
	if ((box.max.b - box.min.b) > (box.max.g - box.min.g)) {
		longestAxis = 2;
	}

	int median = medianCutFindMedian(box.pixels, longestAxis);
	ColorBox box1, box2;
	box1.pixels.reserve(box.pixels.size());
	box2.pixels.reserve(box.pixels.size());
	for (const color::RGBA &color : box.pixels) {
		if (longestAxis == 0) {
			if (color.r < median) {
				box1.pixels.push_back(color);
			} else {
				box2.pixels.push_back(color);
			}
		} else if (longestAxis == 1) {
			if (color.g < median) {
				box1.pixels.push_back(color);
			} else {
				box2.pixels.push_back(color);
			}
		} else {
			if (color.b < median) {
				box1.pixels.push_back(color);
			} else {
				box2.pixels.push_back(color);
			}
		}
	}

	box1.min.r = box.min.r;
	box1.max.r = box.max.r;
	box1.min.g = box.min.g;
	box1.max.g = box.max.g;
	box1.min.b = box.min.b;
	box1.max.b = box.max.b;

	box2.min.r = box.min.r;
	box2.max.r = box.max.r;
	box2.min.g = box.min.g;
	box2.max.g = box.max.g;
	box2.min.b = box.min.b;
	box2.max.b = box.max.b;

	if (longestAxis == 0) {
		box1.max.r = median;
		box2.min.r = median;
	} else if (longestAxis == 1) {
		box1.max.g = median;
		box2.min.g = median;
	} else {
		box1.max.b = median;
		box2.min.b = median;
	}

	return core::Pair{box1, box2};
}

static int quantizeMedianCut(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors) {
	core::DynamicArray<ColorBox> boxes;
	boxes.reserve(maxTargetBufColors + 1);
	{
		color::RGBA white{0, 0, 0, 255};
		color::RGBA black{255, 255, 255, 255};
		core::Buffer<RGBA> pixels;
		pixels.append(inputBuf, inputBufColors);
		boxes.emplace_back(white, black, core::move(pixels));
	}

	while (boxes.size() < maxTargetBufColors) {
		size_t maxSize = 0;
		size_t maxIndex = 0;
		for (size_t i = 0; i < boxes.size(); ++i) {
			if (boxes[i].pixels.size() > maxSize) {
				maxSize = boxes[i].pixels.size();
				maxIndex = i;
			}
		}

		// Split the most populated box.
		core::Pair<ColorBox, ColorBox> boxesPair = medianCutSplitBox(boxes[maxIndex]);
		boxes.erase(maxIndex);
		boxes.push_back(boxesPair.first);
		boxes.push_back(boxesPair.second);
	}

	size_t n = 0;
	for (const ColorBox &box : boxes) {
		if (box.pixels.empty()) {
			continue;
		}
		uint32_t r = 0, g = 0, b = 0, a = 0;
		for (const color::RGBA &color : box.pixels) {
			r += color.r;
			g += color.g;
			b += color.b;
			a += color.a;
		}
		r /= box.pixels.size();
		g /= box.pixels.size();
		b /= box.pixels.size();
		a /= box.pixels.size();
		targetBuf[n++] = color::RGBA(r, g, b, a);
		if (n >= maxTargetBufColors) {
			return (int)n;
		}
	}
	for (size_t i = n; i < maxTargetBufColors; ++i) {
		targetBuf[i] = RGBA(0xFFFFFFFFU);
	}
	return (int)n;
}

static int quantizeOctree(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors) {
	core_assert(glm::isPowerOfTwo(maxTargetBufColors));
	using BBox = math::AABB<uint8_t>;
	struct ColorNode {
		inline ColorNode(color::RGBA c) : color(c) {};
		color::RGBA color;
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
	const glm::ivec3 dim(8);
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

static inline float getDistance(const glm::vec4 &p1, const glm::vec4 &p2) {
	const glm::aligned_vec4 v = p1 - p2;
	return glm::dot(v, v);
}

static int quantizeKMeans(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors) {
	core::Buffer<glm::vec4> centers;
	centers.reserve(maxTargetBufColors);
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, (int)inputBufColors - 1);
	for (int i = 0; i < (int)maxTargetBufColors; i++) {
		centers.emplace_back(color::fromRGBA(inputBuf[dis(gen)]));
	}

	bool changed = true;
	while (changed) {
		changed = false;
		core::DynamicArray<core::Buffer<glm::vec4>> clusters(maxTargetBufColors);
		for (size_t i = 0; i < inputBufColors; ++i) {
			const glm::vec4 point = color::fromRGBA(inputBuf[i]);
			int closest = 0;
			float closestDistance = getDistance(point, centers[0]);
			for (int n = 1; n < (int)maxTargetBufColors; n++) {
				float d = getDistance(point, centers[n]);
				if (d < closestDistance) {
					closest = n;
					closestDistance = d;
				}
			}
			clusters[closest].push_back(point);
		}
		for (int i = 0; i < (int)maxTargetBufColors; i++) {
			if (clusters[i].empty()) {
				continue;
			}
			glm::vec4 newCenter(0.0f);
			for (const glm::vec4 &point : clusters[i]) {
				newCenter += point;
			}
			newCenter /= clusters[i].size();
			if (getDistance(newCenter, centers[i]) > 0.0001f) {
				centers[i] = newCenter;
				changed = true;
			}
		}
	}

	size_t n = 0;
	for (const glm::vec4 &c : centers) {
		targetBuf[n++] = color::toRGBA(c);
	}
	for (size_t i = n; i < maxTargetBufColors; ++i) {
		targetBuf[i] = RGBA(0xFFFFFFFFU);
	}
	return (int)n;
}

// Based on NeuQuant algorithm from jo_gif_quantize
static int quantizeNeuQuant(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors) {
	const int numColors = (int)maxTargetBufColors;
	if (numColors == 0) {
		return 0;
	}
	const int rgbaSize = (int)inputBufColors;
	int sample = 1;

	// defs for freq and bias
	const int intbiasshift = 16; /* bias for fractions */
	const int intbias = (((int)1) << intbiasshift);
	const int gammashift = 10; /* gamma = 1024 */
	const int betashift = 10;
	const int beta = (intbias >> betashift); /* beta = 1/1024 */
	const int betagamma = (intbias << (gammashift - betashift));

	// defs for decreasing radius factor
	const int radiusbiasshift = 6; /* at 32.0 biased by 6 bits */
	const int radiusbias = (((int)1) << radiusbiasshift);
	const int radiusdec = 30; /* factor of 1/30 each cycle */

	// defs for decreasing alpha factor
	const int alphabiasshift = 10; /* alpha starts at 1.0 */
	const int initalpha = (((int)1) << alphabiasshift);

	// radbias and alpharadbias used for radpower calculation
	const int radbiasshift = 8;
	const int radbias = (((int)1) << radbiasshift);
	const int alpharadbshift = (alphabiasshift + radbiasshift);
	const int alpharadbias = (((int)1) << alpharadbshift);

	sample = glm::clamp(sample, 1, 30);
	int network[256][3];
	int bias[256] = {}, freq[256];
	for (int i = 0; i < numColors; ++i) {
		// Put nurons evenly through the luminance spectrum.
		network[i][0] = network[i][1] = network[i][2] = (i << 12) / numColors;
		freq[i] = intbias / numColors;
	}
	// Learn
	{
		const int primes[5] = {499, 491, 487, 503};
		int step = 4;
		for (int i = 0; i < 4; ++i) {
			if (rgbaSize > primes[i] * 4 && (rgbaSize % primes[i])) { // TODO/Error? primes[i]*4?
				step = primes[i] * 4;
			}
		}
		sample = step == 4 ? 1 : sample;

		int alphadec = 30 + ((sample - 1) / 3);
		int samplepixels = rgbaSize / (4 * sample);
		int delta = samplepixels / 100;
		int alpha = initalpha;
		delta = delta == 0 ? 1 : delta;

		int radius = (numColors >> 3) * radiusbias;
		int rad = radius >> radiusbiasshift;
		rad = rad <= 1 ? 0 : rad;
		int radSq = rad * rad;
		int radpower[32];
		for (int i = 0; i < rad; i++) {
			radpower[i] = alpha * (((radSq - i * i) * radbias) / radSq);
		}

		// Randomly walk through the pixels and relax neurons to the "optimal" target.
		for (int i = 0, pix = 0; i < samplepixels;) {
			int r = inputBuf[pix].r << 4;
			int g = inputBuf[pix].g << 4;
			int b = inputBuf[pix].b << 4;
			int j = -1;
			{
				// finds closest neuron (min dist) and updates freq
				// finds best neuron (min dist-bias) and returns position
				// for frequently chosen neurons, freq[k] is high and bias[k] is negative
				// bias[k] = gamma*((1/numColors)-freq[k])

				int bestd = 0x7FFFFFFF, bestbiasd = 0x7FFFFFFF, bestpos = -1;
				for (int k = 0; k < numColors; k++) {
					int *n = network[k];
					int dist = abs(n[0] - r) + abs(n[1] - g) + abs(n[2] - b);
					if (dist < bestd) {
						bestd = dist;
						bestpos = k;
					}
					int biasdist = dist - ((bias[k]) >> (intbiasshift - 4));
					if (biasdist < bestbiasd) {
						bestbiasd = biasdist;
						j = k;
					}
					int betafreq = freq[k] >> betashift;
					freq[k] -= betafreq;
					bias[k] += betafreq << gammashift;
				}
				freq[bestpos] += beta;
				bias[bestpos] -= betagamma;
			}

			// Move neuron j towards biased (b,g,r) by factor alpha
			network[j][0] -= (network[j][0] - r) * alpha / initalpha;
			network[j][1] -= (network[j][1] - g) * alpha / initalpha;
			network[j][2] -= (network[j][2] - b) * alpha / initalpha;
			if (rad != 0) {
				// Move adjacent neurons by precomputed alpha*(1-((i-j)^2/[r]^2)) in radpower[|i-j|]
				int lo = j - rad;
				lo = lo < -1 ? -1 : lo;
				int hi = j + rad;
				hi = hi > numColors ? numColors : hi;
				for (int jj = j + 1, m = 1; jj < hi; ++jj) {
					int a = radpower[m++];
					network[jj][0] -= (network[jj][0] - r) * a / alpharadbias;
					network[jj][1] -= (network[jj][1] - g) * a / alpharadbias;
					network[jj][2] -= (network[jj][2] - b) * a / alpharadbias;
				}
				for (int k = j - 1, m = 1; k > lo; --k) {
					int a = radpower[m++];
					network[k][0] -= (network[k][0] - r) * a / alpharadbias;
					network[k][1] -= (network[k][1] - g) * a / alpharadbias;
					network[k][2] -= (network[k][2] - b) * a / alpharadbias;
				}
			}

			pix += step;
			pix = pix >= rgbaSize ? pix - rgbaSize : pix;

			// every 1% of the image, move less over the following iterations.
			if (++i % delta == 0) {
				alpha -= alpha / alphadec;
				radius -= radius / radiusdec;
				rad = radius >> radiusbiasshift;
				rad = rad <= 1 ? 0 : rad;
				radSq = rad * rad;
				for (j = 0; j < rad; j++) {
					radpower[j] = alpha * ((radSq - j * j) * radbias / radSq);
				}
			}
		}
	}
	// Unbias network to give byte values 0..255
	for (int i = 0; i < numColors; i++) {
		targetBuf[i].r = network[i][0] >>= 4;
		targetBuf[i].g = network[i][1] >>= 4;
		targetBuf[i].b = network[i][2] >>= 4;
		targetBuf[i].a = 255;
	}
	return numColors;
}

static int quantizeWu(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors) {
	// Initialize the set of boxes with the full color range
	core::DynamicArray<ColorBox> boxes;
	boxes.reserve(maxTargetBufColors + 1);
	{
		core::Buffer<RGBA> pixels;
		pixels.append(inputBuf, inputBufColors);
		color::RGBA white{0, 0, 0, 255};
		color::RGBA black{255, 255, 255, 255};
		boxes.emplace_back(white, black, core::move(pixels));
	}

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
		const ColorBox &box = boxes[maxVolumeIndex];
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

		ColorBox box1, box2;
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
		default:
			break;
		}

		// Replace the original box with the two split boxes
		boxes.erase(maxVolumeIndex);
		boxes.emplace_back(core::move(box1));
		boxes.emplace_back(core::move(box2));
	}

	size_t n = 0;
	for (const ColorBox &box : boxes) {
		if (box.pixels.empty()) {
			continue;
		}
		RGBA average(0, 0, 0, 255);
		for (const RGBA &pixel : box.pixels) {
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

int quantize(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors,
			 ColorReductionType type) {
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
	case ColorReductionType::KMeans:
		return quantizeKMeans(targetBuf, maxTargetBufColors, inputBuf, inputBufColors);
	case ColorReductionType::NeuQuant:
		return quantizeNeuQuant(targetBuf, maxTargetBufColors, inputBuf, inputBufColors);
	case ColorReductionType::Octree:
		return quantizeOctree(targetBuf, maxTargetBufColors, inputBuf, inputBufColors);
	case ColorReductionType::MedianCut:
		return quantizeMedianCut(targetBuf, maxTargetBufColors, inputBuf, inputBufColors);
	default:
		break;
	}
	return -1;
}

} // namespace color
