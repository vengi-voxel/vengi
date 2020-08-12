/**
 * @file
 */
#pragma once

#include "core/collection/Vector.h"
#include "video/Texture.h"
#include "noise/Noise.h"
#include "video/Types.h"
#include "core/IComponent.h"

#include <future>

namespace render {

/**
 * @brief Generates a seamless random colored texture
 */
class RandomColorTexture : public core::IComponent {
private:
	video::TexturePtr _colorTexture;
	noise::Noise _noise;

	struct NoiseGenerationTask {
		NoiseGenerationTask() : buffer(nullptr), width(0), height(0), depth(0) {
		}
		NoiseGenerationTask(uint8_t *_buffer, int _width, int _height, int _depth) :
				buffer(_buffer), width(_width), height(_height), depth(_depth) {
		}
		/** @brief pointer to preallocated buffer that was hand into the noise task */
		uint8_t *buffer;
		int width;
		int height;
		int depth;
	};

	typedef std::future<NoiseGenerationTask> NoiseFuture;
	core::Vector<NoiseFuture, 1> _noiseFuture;
public:
	bool init() override;
	void shutdown() override;

	video::Id handle() const;

	void bind(video::TextureUnit unit = video::TextureUnit::Zero);
	void unbind();
};

}
