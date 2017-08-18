/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "noise/Noise.h"
#include "image/Image.h"
#include "core/GLM.h"

namespace noise {

class NoiseTest: public core::AbstractTest {
protected:
	template<typename NOISE>
	void test2DNoise(NOISE&& noiseFunc, float frequency, const char *filename, int w = 256, int h = 256, int components = 4) {
		uint8_t* buffer = new uint8_t[w * h * components];

		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const glm::vec2 pos(x * frequency, y * frequency);
				const float noise = noiseFunc(pos);
//				ASSERT_LE(noise, +1.0f)<< "Noise is bigger than 1.0: " << noise;
//				ASSERT_GE(noise, -1.0f)<< "Noise is less than -1.0: " << noise;
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
		ASSERT_TRUE(image::Image::writePng(filename, buffer, w, h, components));
		delete[] buffer;
	}
};

TEST_F(NoiseTest, testHumidityNoise) {
	test2DNoise([] (const glm::vec2& pos) {return noise::noise(pos);}, 0.001f, "testHumidity.png");
}

TEST_F(NoiseTest, testTemperatureNoise) {
	test2DNoise([] (const glm::vec2& pos) {return noise::noise(pos); }, 0.0001f, "testTemperature.png");
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

TEST_F(NoiseTest, test2DNoiseRF) {
	test2DNoise([] (const glm::vec2& pos) {return noise::ridgedMF(pos, 128.0f, 4, 2.02f, 1.0f);}, 20.0f, "test-ridgedmf-noise-1024-2048.png", 1024, 2048, 4);
}

}
