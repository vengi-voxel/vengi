/**
 * @file
 */

#include "Cubemap.h"
#include "core/Log.h"
#include "core/String.h"
#include "image/Image.h"

namespace video {

Cubemap::Cubemap(const std::string& filename) :
		_filename(filename), _textureHandle(InvalidId) {
}

Cubemap::~Cubemap() {
	core_assert_msg(_textureHandle == InvalidId, "Cubemap was not properly shut down");
	shutdown();
}

void Cubemap::shutdown() {
	video::deleteTexture(_textureHandle);
}

bool Cubemap::load() {
	_textureHandle = video::genTexture();
	image::ImagePtr images[6];
	for (unsigned int i = 0; i < 6; ++i) {
		const std::string& filename = core::string::format("%s-cm-%i", _filename.c_str(), i + 1);
		images[i] = image::loadImage(filename);
	}
	return video::setupCubemap(_textureHandle, images);
}

void Cubemap::bind(video::TextureUnit texUnit) {
	video::bindTexture(texUnit, video::TextureType::TextureCube, _textureHandle);
}

void Cubemap::unbind(video::TextureUnit texUnit) {
	video::bindTexture(texUnit, video::TextureType::TextureCube, video::InvalidId);
}

}
