/**
 * @file
 */

#include "Cubemap.h"
#include "core/Log.h"
#include "core/String.h"
#include "image/Image.h"

namespace video {

Cubemap::Cubemap(const std::string& filename) :
		_filename(filename), _textureHandle(0u) {
}

Cubemap::~Cubemap() {
	core_assert_msg(_textureHandle == 0u, "Cubemap was not properly shut down");
	shutdown();
}

void Cubemap::shutdown() {
	if (_textureHandle != 0) {
		glDeleteTextures(1, &_textureHandle);
		_textureHandle = 0u;
	}
}

bool Cubemap::load() {
	glGenTextures(1, &_textureHandle);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _textureHandle);

	static const GLenum types[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	for (unsigned int i = 1; i <= 6; i++) {
		const std::string& filename = core::string::format("%s-cm-%i", _filename.c_str(), i);
		const image::ImagePtr& img = image::loadImage(filename);
		const GLenum mode = img->depth() == 4 ? GL_RGBA : GL_RGB;
		glTexImage2D(types[i - 1], 0, mode, img->width(), img->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data());
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return true;
}

void Cubemap::bind(video::TextureUnit texUnit) {
	glActiveTexture(std::enum_value(texUnit));
	glBindTexture(GL_TEXTURE_CUBE_MAP, _textureHandle);
}

void Cubemap::unbind(video::TextureUnit texUnit) {
	glActiveTexture(std::enum_value(texUnit));
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

}
