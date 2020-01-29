/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "compute/Compute.h"
#include "noise/Noise.h"
#include "image/Image.h"
#include "core/GLM.h"
#include "core/StringUtil.h"

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
