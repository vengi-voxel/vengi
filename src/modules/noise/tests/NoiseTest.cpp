/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "noise/Noise.h"
#include "noise/Simplex.h"
// TODO: not a real dependency to the voxel module.... but... look there - a three headed monkey!
#include "voxel/WorldContext.h"
#include "voxel/Constants.h"
#include "image/Image.h"
#include "core/GLM.h"

namespace noise {

class NoiseTest: public core::AbstractTest {
protected:
	const int components = 4;
	const int w = 256;
	const int h = 256;

	bool WriteImage(const char* name, uint8_t* buffer, int w = 256, int h = 256, int components = 4) {
		return image::Image::writePng(name, buffer, w, h, components);
	}

	void test2DNoise(int octaves, float lacunarity, float frequency, const char *filename, float gain = 0.5f) {
		uint8_t buffer[w * h * components];

		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const glm::vec2 pos(x * frequency, y * frequency);
				const float noise = noise::ridgedMF(pos, 1.0f, octaves, lacunarity, gain);
				ASSERT_LE(noise, +1.0f)<< "Noise is bigger than 1.0: " << noise;
				ASSERT_GE(noise, -1.0f)<< "Noise is less than -1.0: " << noise;
				float normalized = noise::norm(noise);
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
				if (components == 4) {
					buffer[index] = 255;
				}
			}
		}
		ASSERT_TRUE(WriteImage(filename, buffer));
	}
};

TEST_F(NoiseTest, testHumidityNoise) {
	test2DNoise(1, 1.0f, 0.001f, "testHumidity.png");
}

TEST_F(NoiseTest, testTemperatureNoise) {
	test2DNoise(1, 1.0f, 0.01f, "testTemperature.png");
}

TEST_F(NoiseTest, test2DNoiseColorMap) {
	const int width = 256;
	const int height = 256;
	const int components = 3;
	uint8_t buffer[width * height * components];
	const int octaves = 2;
	const float persistence = 0.3f;
	const float frequency = 0.7f;
	const float amplitude = 1.0f;
	noise::SeamlessNoise2DRGB(buffer, width, octaves, persistence, frequency, amplitude);
	ASSERT_TRUE(image::Image::writePng("testNoiseColorMap.png", buffer, width, height, components));
}

}
