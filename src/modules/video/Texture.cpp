/**
 * @file
 */

#include "Texture.h"
#include "core/Log.h"
#include "core/String.h"

namespace video {

static const struct Formats {
	uint8_t bits;
	GLenum internalFormat;
	GLenum dataFormat;
	GLenum dataType;
} textureFormats[] = {
	{32, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},
	{24, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE},
	{32, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8}
};

Texture::Texture(TextureType type, TextureFormat format, const std::string& name, int width, int height, int index, TextureWrap wrap) :
		io::IOResource(), _name(name), _width(width), _height(height), _type(type), _format(format), _wrap(wrap) {
}

Texture::~Texture() {
	// in case of a texture we don't want this check, as it might be shared between multiple resources
	// and it should only be destroyed once it's completely destroyed by the shared_ptr
	//core_assert_msg(_handle == 0u, "Texture %s was not properly shut down", _name.c_str());
	shutdown();
}

void Texture::shutdown() {
	if (_handle != 0) {
		glDeleteTextures(1, &_handle);
		_handle = 0;
	}
}

void Texture::upload(TextureFormat format, int width, int height, const uint8_t* data, int index) {
	_format = format;
	upload(width, height, data, index);
}

void Texture::upload(const uint8_t* data, int index) {
	upload(_width, _height, data, index);
}

void Texture::upload(int width, int height, const uint8_t* data, int index) {
	if (_handle == 0u) {
		glGenTextures(1, &_handle);
	}
	bind(TextureUnit::Upload);
	_width = width;
	_height = height;
	const Formats& f = textureFormats[std::enum_value(_format)];
	const GLenum gltype = std::enum_value(_type);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (_type == TextureType::Texture2D) {
		glTexImage2D(gltype, 0, f.internalFormat, _width, _height, 0, f.dataFormat, f.dataType, (const void*)data);
	} else {
		glTexImage3D(gltype, 0, f.internalFormat, _width, _height, index, 0, f.dataFormat, f.dataType, (const void*)data);
	}

	glTexParameteri(gltype, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(gltype, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(gltype, GL_TEXTURE_WRAP_S, std::enum_value(_wrap));
	glTexParameteri(gltype, GL_TEXTURE_WRAP_T, std::enum_value(_wrap));
	glTexParameteri(gltype, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(gltype, GL_TEXTURE_MAX_LEVEL, 0);
	_state = io::IOSTATE_LOADED;
	unbind();
	GL_checkError();
}

void Texture::bind(TextureUnit unit) const {
	glActiveTexture(std::enum_value(unit));
	glBindTexture(std::enum_value(_type), _handle);
	GL_checkError();
	_boundUnit = unit;
}

void Texture::unbind() const {
	glActiveTexture(std::enum_value(_boundUnit));
	glBindTexture(std::enum_value(_type), 0);
	GL_checkError();
	_boundUnit = TextureUnit::Zero;
}

}
