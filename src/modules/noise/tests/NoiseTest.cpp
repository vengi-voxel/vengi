/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "compute/Compute.h"
#include "noise/Noise.h"
#include "image/Image.h"
#include "core/GLM.h"

namespace noise {

class NoiseTest: public core::AbstractTest {
protected:
	void onCleanupApp() override {
		compute::shutdown();
	}

	bool onInitApp() override {
		compute::init();
		return true;
	}

	template<typename NOISE>
	void test2DNoise(NOISE&& noiseFunc, float frequency, const char *filename, int w = 256, int h = 256, int components = 4) {
		uint8_t* buffer = new uint8_t[w * h * components];

		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const glm::vec2 pos(x * frequency, y * frequency);
				const float noise = noiseFunc(pos);
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

	void seamlessNoise(bool useShader) {
		noise::Noise noise;
		ASSERT_TRUE(noise.init());
		noise.useShader(useShader);
		if (useShader && !noise.canUseShader()) {
			return;
		}
		const int width = 256;
		const int height = 256;
		const int components = 3;
		uint8_t buffer[width * height * components];
		const int octaves = 2;
		const float persistence = 0.3f;
		const float frequency = 0.7f;
		const float amplitude = 1.0f;
		noise.seamlessNoise(buffer, width, octaves, persistence, frequency, amplitude);
		const std::string& target = core::string::format("testseamlessNoise-%i.png", useShader ? 1 : 0);
		EXPECT_TRUE(image::Image::writePng(target.c_str(), buffer, width, height, components));
		noise.shutdown();
	}
};

TEST_F(NoiseTest, testSeamlessNoiseShader) {
	seamlessNoise(true);
}

TEST_F(NoiseTest, testSeamlessNoiseCPU) {
	seamlessNoise(false);
}

}
