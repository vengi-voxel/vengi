/**
 * @file
 */
#include "RandomColorTexture.h"
#include "core/App.h"
#include "NoiseShaders.h"

namespace frontend {

bool RandomColorTexture::init() {
	_colorTexture = video::createEmptyTexture("**colortexture**");
	compute::NoiseShader shader;
	const int ColorTextureSize = 256;
	const int ColorTextureOctaves = 2;
	const int ColorTextureDepth = 3;
	const float persistence = 0.3f;
	const float frequency = 0.7f;
	const float amplitude = 1.0f;
	if (shader.setup()) {
		const glm::ivec2 workSize(ColorTextureSize);
		std::vector<uint8_t> colorTexture(ColorTextureSize * ColorTextureSize * ColorTextureDepth);
		shader.seamlessNoise(colorTexture, ColorTextureSize, ColorTextureOctaves, persistence, frequency, amplitude, workSize);
		_colorTexture->upload(video::TextureFormat::RGB, workSize.x, workSize.y, &colorTexture[0]);
		return true;
	}
	_noiseFuture.push_back(core::App::getInstance()->threadPool().enqueue([=] () {
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		_noise.seamlessNoise2DRGB(colorTexture, ColorTextureSize, ColorTextureOctaves, persistence, frequency, amplitude);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, ColorTextureDepth);
	}));
	return true;
}

void RandomColorTexture::bind(video::TextureUnit unit) {
	if (!_noiseFuture.empty()) {
		NoiseFuture& future = _noiseFuture.back();
		if (future.valid()) {
			NoiseGenerationTask c = future.get();
			Log::trace("Noise texture ready - upload it");
			video::TextureFormat format;
			if (c.depth == 4) {
				format = video::TextureFormat::RGBA;
			} else {
				format = video::TextureFormat::RGB;
			}
			_colorTexture->upload(format, c.width, c.height, c.buffer);
			delete[] c.buffer;
			_noiseFuture.pop_back();
		}
	}
	_colorTexture->bind(unit);
}

void RandomColorTexture::unbind() {
	_colorTexture->unbind();
}

void RandomColorTexture::shutdown() {
	if (_colorTexture) {
		_colorTexture->shutdown();
		_colorTexture = video::TexturePtr();
	}
	_noiseFuture.clear();
}

}
