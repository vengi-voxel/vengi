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
	constexpr int dim = 256;
	const size_t minsize = dim * dim * sizeof(uint32_t);
	size_t bufSize = minsize;
	uint32_t *buf = (uint32_t*)shader.bufferAlloc(bufSize);
	ASSERT_TRUE(buf != nullptr);
	ASSERT_TRUE(bufSize >= minsize);
	const float scale[] = { 20.0f, 20.0f };
	const float bias[] = { 128.0f, 128.0f };
	const float lacunarity = 2.02f;
	const float increment = 1.0f;
	const float octaves = 3.3f;
	const float amplitude = 1.0f;

	ASSERT_TRUE(shader.ridgedMF(
			(uint8_t* )buf, bufSize, bias, scale,
			lacunarity, increment, octaves, amplitude, dim, 2));

	ASSERT_TRUE(image::Image::writePng("compute-ridgedmf-noise.png", (const uint8_t*)buf, dim, dim, sizeof(uint32_t)));
	ASSERT_TRUE(buf[0] != 0) << buf[0];
	shader.bufferFree(buf);
	shader.shutdown();
}

}
