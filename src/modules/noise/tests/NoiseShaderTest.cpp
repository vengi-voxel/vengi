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
		const int components = 4;
		const size_t bufSize = width * height * components;
		std::vector<uint8_t> buf(bufSize);
		const float frequency = 20.0f;
		const float lacunarity = 2.02f;
		const uint8_t octaves = 4u;
		const float amplitude = 1.0f;
		const float ridgeOffset = 0.0f;
		const float gain = 1.0f;
		const glm::ivec2 workSize(width, height);

		ASSERT_TRUE(shader.ridgedMF2(buf, components, frequency, amplitude, ridgeOffset,
				octaves, lacunarity, gain, workSize));

		const std::string& imageName = core::string::format("test-compute-ridgedmf-noise-%i-%i.png", width, height);
		ASSERT_TRUE(image::Image::writePng(imageName.c_str(), (const uint8_t*)&buf[0], width, height, components));
		ASSERT_TRUE(buf[0] != 0) << buf[0];
		shader.shutdown();
	}
};

TEST_F(NoiseShaderTest, testNoiseShaderRidgedMultiFractal) {
	generateNoise(256, 256);
}

TEST_F(NoiseShaderTest, testNoiseShaderRidgedMultiFractalUneven) {
	generateNoise(128, 256);
}

TEST_F(NoiseShaderTest, testNoiseShaderSeamless) {
	if (!_supported) {
		return;
	}
	compute::NoiseShader shader;
	ASSERT_TRUE(shader.setup());
	const int width = 512;
	const int components = 3;
	const size_t bufSize = width * width * components;
	std::vector<uint8_t> buf(bufSize);
	const float gain = 1.0f;
	const float lacunarity = 2.02f;
	const uint8_t octaves = 4u;
	const glm::ivec2 workSize(width, width);

	ASSERT_TRUE(shader.seamlessNoise(buf, width, components, octaves, lacunarity, gain, workSize));

	const std::string& imageName = core::string::format("test-compute-seamsless-noise-%i-%i.png", width, width);
	ASSERT_TRUE(image::Image::writePng(imageName.c_str(), (const uint8_t*)&buf[0], width, width, components));
	ASSERT_TRUE(buf[0] != 0) << buf[0];
	shader.shutdown();
}

}
