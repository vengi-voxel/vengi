/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "io/FileStream.h"
#include "noise/Noise.h"
#include "image/Image.h"
#include "core/GLM.h"
#include "core/StringUtil.h"

namespace noise {

class NoiseTest: public app::AbstractTest {
protected:
	void onCleanupApp() override {
	}

	bool onInitApp() override {
		return true;
	}

	void seamlessNoise() {
		noise::Noise noise;
		ASSERT_TRUE(noise.init());
		const int width = 256;
		const int height = 256;
		const int components = 3;
		uint8_t buffer[width * height * components];
		const int octaves = 2;
		const float persistence = 0.3f;
		const float frequency = 0.7f;
		const float amplitude = 1.0f;
		noise.seamlessNoise(buffer, width, octaves, persistence, frequency, amplitude);
		const io::FilePtr &file = io::filesystem()->open("testseamlessNoise.png", io::FileMode::SysWrite);
		ASSERT_TRUE(file->validHandle());
		io::FileStream stream(file);
		EXPECT_TRUE(image::Image::writePng(stream, buffer, width, height, components));
		noise.shutdown();
	}
};

TEST_F(NoiseTest, testSeamlessNoise) {
	seamlessNoise();
}

}
