#include "Texture.h"
#include "core/Log.h"
#include "core/String.h"

namespace video {

Texture::Texture(const std::string& filename) :
		io::IOResource(), _filename(filename), _textureHandle(0u) {
}

Texture::Texture(uint8_t* data, int width, int height, int depth) :
		io::IOResource(), _filename("mem"), _textureHandle(0u) {
	glGenTextures(1, &_textureHandle);
	glBindTexture(GL_TEXTURE_2D, _textureHandle);
	const GLenum mode = depth == 4 ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

Texture::~Texture() {
	if (_textureHandle != 0) {
		glDeleteTextures(1, &_textureHandle);
	}
}

void Texture::upload() {
	if (!_img)
		return;
	if (_img->isLoaded()) {
		const GLenum mode = _img->hasAlpha() ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, mode, _img->width(), _img->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _img->data());
		_img = ImagePtr();
		_state = io::IOSTATE_LOADED;
	} else if (_img->isFailed()) {
		_state = io::IOSTATE_FAILED;
		_img = ImagePtr();
	}
}

void Texture::load() {
	glGenTextures(1, &_textureHandle);
	glBindTexture(GL_TEXTURE_2D, _textureHandle);
	static const int empty = 0x00000000;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &empty);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	_img = ImagePtr(new Image(_filename));
	_img->loadAsync();
}

void Texture::bind() {
	glBindTexture(GL_TEXTURE_2D, _textureHandle);
	upload();
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

}
