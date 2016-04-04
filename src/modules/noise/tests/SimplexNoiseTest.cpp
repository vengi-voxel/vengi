#include "core/tests/AbstractTest.h"
#include "noise/SimplexNoise.h"
#include "image/Image.h"
#include <glm/gtc/noise.hpp>
#include <glm/gtc/constants.hpp>

namespace noise {

class SimplexNoiseTest: public core::AbstractTest {
protected:
	const int components = 4;
	const int w = 256;
	const int h = 256;

	bool WriteImage(const char* name, uint8_t* buffer) {
		return image::Image::writePng(name, buffer, w, h, components);
	}
};

TEST_F(SimplexNoiseTest, testLandscapeMountains) {
	uint8_t buffer[w * h * components];

	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			const glm::vec2 pos(x, y);
			float landscapeNoise = noise::Simplex::Noise2D(pos, 4, 0.3f, 0.01f);
			ASSERT_LE(landscapeNoise, 1.0f)<< "Noise is bigger than 1.0: " << landscapeNoise;
			ASSERT_GE(landscapeNoise, -1.0f)<< "Noise is less than -1.0: " << landscapeNoise;
			float noiseNormalized = (landscapeNoise + 1.0f) * 0.5f;
			ASSERT_LE(noiseNormalized, 1.0f)<< "Noise is bigger than 1.0: " << noiseNormalized;
			ASSERT_GE(noiseNormalized, 0.0f)<< "Noise is less than 0.0: " << noiseNormalized;
			float mountainNoise = noise::Simplex::Noise2D(pos, 4, 0.3f, 0.0075f);
			float mountainNoiseNormalized = (mountainNoise + 1.0f) * 0.5f;
			float mountainMultiplier = mountainNoiseNormalized * 2.3f;
			float noiseHeight = glm::clamp(noiseNormalized * mountainMultiplier, 0.0f, 1.0f);
			unsigned char color = (unsigned char) (noiseHeight * 255.0f);
			ASSERT_LE(color, 255)<< "Color is bigger than 255: " << color;
			ASSERT_GE(color, 0)<< "Color is less than 0: " << color;
			int index = y * (w * components) + (x * components);
			const int n = components == 4 ? 3 : components;
			for (int i = 0; i < n; ++i) {
				buffer[index++] = color;
			}
			if (components == 4)
				buffer[index] = 255;
		}
	}
	ASSERT_TRUE(WriteImage("testNoiseLandscapeMountains.png", buffer));
}

TEST_F(SimplexNoiseTest, test2DNoise) {
	uint8_t buffer[w * h * components];

	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			const glm::vec2 pos(x, y);
			const float noise = Simplex::Noise2D(pos, 2, 1.0f, 0.5f, 1.5f);
			ASSERT_LE(noise, 1.0f)<< "Noise is bigger than 1.0: " << noise;
			ASSERT_GE(noise, -1.0f)<< "Noise is less than -1.0: " << noise;
			float normalized = glm::clamp(noise * 0.5f + 0.5f, 0.0f, 1.0f);
			ASSERT_LE(normalized, 1.0f)<< "Noise is bigger than 1.0: " << normalized;
			ASSERT_GE(normalized, 0.0f)<< "Noise is less than 0.0: " << normalized;
			unsigned char color = (unsigned char) (normalized * 255.0f);
			ASSERT_LE(color, 255)<< "Color is bigger than 255: " << color;
			ASSERT_GE(color, 0)<< "Color is less than 0: " << color;
			int index = y * (w * components) + (x * components);
			const int n = components == 4 ? 3 : components;
			for (int i = 0; i < n; ++i) {
				buffer[index++] = color;
			}
			if (components == 4)
				buffer[index] = 255;
		}
	}
	ASSERT_TRUE(WriteImage("testNoise2d.png", buffer));
}

TEST_F(SimplexNoiseTest, test2DNoiseGray) {
	const int width = 100;
	const int height = 100;
	const int components = 3;
	uint8_t buffer[width * height * components];
	Simplex::Noise2DGray(buffer, width, height, 1, 1.0, 1.0, 1.0);
	ASSERT_TRUE(image::Image::writePng("testNoiseGray.png", buffer, width, height, components));
}

TEST_F(SimplexNoiseTest, test2DNoiseColorMap) {
	const int width = 256;
	const int height = 256;
	const int components = 3;
	uint8_t buffer[width * height * components];
	noise::Simplex::SeamlessNoise2DRGB(buffer, width, 3, 0.3f, 0.7f);
	ASSERT_TRUE(image::Image::writePng("testNoiseColorMap.png", buffer, width, height, components));
}

}
