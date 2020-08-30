/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "noise/Noise.h"
#include "noise/Simplex.h"
#include "image/Image.h"
#include "core/GLM.h"
#include <SDL_endian.h>

namespace noise {

class IslandNoiseTest: public app::AbstractTest {
protected:
	uint32_t biome(float e) {
		if (e < 0.05)
			// water
			return 0x0000ffff;
		else if (e < 0.1)
			// beach
			return 0xffff00ff;
		else if (e < 0.15)
			// beach
			return 0x115500ff;
		else if (e < 0.3)
			// grass
			return 0x008000ff;
		else if (e < 0.35)
			// grass
			return 0x00f800ff;
		else if (e < 0.4)
			// jungle
			return 0x4acb7bff;
		else if (e < 0.45)
			// jungle
			return 0x4fcffbff;
		else if (e < 0.6)
			// savannah
			return 0x804b00ff;
		else if (e < 0.65)
			// savannah
			return 0x502b12ff;
		else if (e < 0.8)
			// desert
			return 0x806f00ff;
		else if (e < 0.85)
			// desert
			return 0x881f10ff;
		// snow
		return 0xffffffff;
	}

};

// http://www.redblobgames.com/maps/terrain-from-noise/#elevation
TEST_F(IslandNoiseTest, DISABLED_testIslandNoise) {
	const int components = 4;
	const int w = 2048;
	const int h = 1024;
	uint8_t* buffer = new uint8_t[w * h * components];

	const bool useTerraces = false;
	const bool useBiomes = true;
	const bool useOctaves = false;
	const float offset = 10.0f;

	// pushes everything up
	const float pushUpwards = 0.02f;
	// pushes the edges down
	const float pushEdgesDownward = 2.0f;
	// controls how quick the drop off is
	// smaller values - earlier drop off
	const float dropOffGradient = 0.865f;

	const float frequency = 2.5f;
	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			const float nx = x / (float) w - 0.5f;
			const float ny = y / (float) h - 0.5f;
			const glm::vec2 pos(nx, ny);
			const float d = glm::sqrt(pos.x * pos.x + pos.y * pos.y);
			float e;
			if (useOctaves) {
				e = noise::fBm((pos + offset) * frequency, 4, 0.5f, 2.0f);
			} else {
				e = noise::noise((pos + offset) * frequency);
			}
			e = noise::norm(e);
			float height = (e + pushUpwards) * (1.0f - pushEdgesDownward * glm::pow(d, dropOffGradient));
			if (useTerraces) {
				// the smaller the value, the bigger the terraces
				const float terraces = 100.0f;
				height = glm::round(height * terraces) / terraces;
			}
			const int index = y * (w * components) + (x * components);
			if (useBiomes) {
				const uint32_t color = SDL_Swap32(biome(height));
				*(uint32_t*)&buffer[index] = color;
			} else {
				const uint8_t color = height * 255.0f;
				buffer[index + 0] = color;
				buffer[index + 1] = color;
				buffer[index + 2] = color;
				buffer[index + 3] = 0xff;
			}
		}
	}
	ASSERT_TRUE(image::Image::writePng("testIslandNoise.png", buffer, w, h, components));
	delete[] buffer;
}

}
