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

	bool WriteImage(const char* name, uint8_t* buffer, int w = 256, int h = 256, int components = 4) {
		return image::Image::writePng(name, buffer, w, h, components);
	}
};

TEST_F(NoiseShaderTest, testNoiseShaderRidgedMultiFractal) {
	if (!_supported) {
		return;
	}
	compute::NoiseShader shader;
	ASSERT_TRUE(shader.setup());
	constexpr int size = 256;
	const size_t bufSize = size * size * sizeof(uint32_t);
	std::vector<uint8_t> buf(bufSize);
	const float frequency = 20.0f;
	const glm::vec2 position(128.0f, 128.0f);
	const float lacunarity = 2.02f;
	const int octaves = 3;
	const float amplitude = 1.0f;

	ASSERT_TRUE(shader.ridgedMF(
			buf, position, frequency,
			lacunarity, octaves, amplitude, size, 2));

	ASSERT_TRUE(image::Image::writePng("compute-ridgedmf-noise.png", (const uint8_t*)&buf[0], size, size, sizeof(uint32_t)));
	ASSERT_TRUE(buf[0] != 0) << buf[0];
	shader.shutdown();
}

}
