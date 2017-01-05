#include "RandomColorTexture.h"

namespace frontend {

void RandomColorTexture::init() {
	_colorTexture = video::createEmptyTexture("**colortexture**");

	_noiseFuture.push_back(core::App::getInstance()->threadPool().enqueue([] () {
		const int ColorTextureSize = 256;
		const int ColorTextureOctaves = 2;
		const int ColorTextureDepth = 3;
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		const float persistence = 0.3f;
		const float frequency = 0.7f;
		const float amplitude = 1.0f;
		noise::Simplex::SeamlessNoise2DRGB(colorTexture, ColorTextureSize, ColorTextureOctaves, persistence, frequency, amplitude);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, ColorTextureDepth);
	}));
}

void RandomColorTexture::bind(int unit) {
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
	_colorTexture->shutdown();
	_colorTexture = video::TexturePtr();
	_noiseFuture.clear();
}

}
