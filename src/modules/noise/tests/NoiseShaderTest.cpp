/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "image/Image.h"
#include "NoiseShaders.h"

namespace noise {

class NoiseShaderTest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	bool _supported = false;
public:
	void SetUp() override {
		Super::SetUp();
		_supported = compute::init();
	}

	void TearDown() override {
		compute::shutdown();
		Super::TearDown();
	}

	void generateNoise(int width, int height) {
		if (!_supported) {
			return;
		}
		compute::NoiseShader shader;
		ASSERT_TRUE(shader.setup());
		const size_t bufSize = width * height * sizeof(uint32_t);
		std::vector<uint8_t> buf(bufSize);
		const float frequency = 20.0f;
		const glm::vec2 position(128.0f, 128.0f);
		const float lacunarity = 2.02f;
		const int octaves = 4;
		const float amplitude = 1.0f;

		ASSERT_TRUE(shader.ridgedMF(
				buf, position, frequency,
				lacunarity, octaves, amplitude, glm::ivec2(width, height)));

		const std::string& imageName = core::string::format("test-compute-ridgedmf-noise-%i-%i.png", width, height);
		ASSERT_TRUE(image::Image::writePng(imageName.c_str(), (const uint8_t*)&buf[0], width, height, sizeof(uint32_t)));
		ASSERT_TRUE(buf[0] != 0) << buf[0];
		shader.shutdown();
	}
};

TEST_F(NoiseShaderTest, testNoiseShaderRidgedMultiFractal) {
	generateNoise(256, 256);
}


TEST_F(NoiseShaderTest, testNoiseShaderRidgedMultiFractalUneven) {
	generateNoise(1024, 2048);
}

}
