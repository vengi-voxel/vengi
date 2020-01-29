/**
 * @file
 */

#include "Cubemap.h"
#include "core/Assert.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "image/Image.h"

namespace video {

Cubemap::~Cubemap() {
	core_assert_msg(_textureHandle == InvalidId, "Cubemap was not properly shut down");
	shutdown();
}

void Cubemap::shutdown() {
	video::deleteTexture(_textureHandle);
	_textureHandle = InvalidId;
	_filename.clear();
}

bool Cubemap::init(const char *filename) {
	_filename = filename;
	_textureHandle = video::genTexture();
	static const char *postfix[] = {
		"rt", "lf", "up", "dn", "ft", "bk"
	};
	image::ImagePtr images[lengthof(postfix)];

	for (int i = 0; i < lengthof(postfix); ++i) {
		const std::string& sidename = core::string::format("%s_%s.png", _filename.c_str(), postfix[i]);
		images[i] = image::loadImage(sidename, false);
		if (!images[i]) {
			Log::error("Could not load cubemap image %s", sidename.c_str());
			return false;
		}
		if (images[i]->width() <= 0 || images[i]->height() <= 0) {
			Log::error("Invalid image dimensions for image %s", sidename.c_str());
			return false;
		}
		if (images[i]->depth() != 4 && images[i]->depth() != 3) {
			Log::error("Unsupported image depth for image %s: %i", sidename.c_str(), images[i]->depth());
			return false;
		}
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
