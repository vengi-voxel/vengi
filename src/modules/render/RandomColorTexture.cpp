/**
 * @file
 */
#include "RandomColorTexture.h"
#include "app/App.h"
#include "core/concurrent/ThreadPool.h"
#include "core/Log.h"

namespace render {

bool RandomColorTexture::init() {
	if (!_noise.init()) {
		return false;
	}
	_colorTexture = video::createEmptyTexture("**colortexture**");
	const int ColorTextureSize = 256;
	const int ColorTextureOctaves = 2;
	const int ColorTextureDepth = 3;
	const float persistence = 0.3f;
	const float frequency = 0.7f;
	const float amplitude = 1.0f;
	if (_noise.canUseShader()) {
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		_noise.seamlessNoise(colorTexture, ColorTextureSize, ColorTextureOctaves, persistence, frequency, amplitude);
		video::TextureFormat format = video::TextureFormat::RGB;
		_colorTexture->upload(format, ColorTextureSize, ColorTextureSize, colorTexture);
		delete[] colorTexture;
		return true;
	}
	_noiseFuture.emplace_back(app::App::getInstance()->threadPool().enqueue([=] () {
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * ColorTextureDepth];
		_noise.seamlessNoise(colorTexture, ColorTextureSize, ColorTextureOctaves, persistence, frequency, amplitude);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, ColorTextureDepth);
	}));
	return true;
}

video::Id RandomColorTexture::handle() const {
	if (!_colorTexture) {
		return video::InvalidId;
	}
	return _colorTexture->handle();
}

void RandomColorTexture::bind(video::TextureUnit unit) {
	if (!_noiseFuture.empty()) {
		NoiseFuture& future = _noiseFuture.front();
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
			_noiseFuture.clear();
		}
	}
	_colorTexture->bind(unit);
}

void RandomColorTexture::unbind() {
	_colorTexture->unbind();
}

void RandomColorTexture::shutdown() {
	while (!_noiseFuture.empty()) {
		NoiseFuture& future = _noiseFuture.front();
		if (future.valid()) {
			NoiseGenerationTask c = future.get();
			delete[] c.buffer;
			_noiseFuture.clear();
		}
	}
	if (_colorTexture) {
		_colorTexture->shutdown();
		_colorTexture = video::TexturePtr();
	}
	_noiseFuture.clear();
	_noise.shutdown();
}

}
