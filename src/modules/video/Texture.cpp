/**
 * @file
 */

#include "Texture.h"
#include "core/Log.h"
#include "core/String.h"
#include "video/GLFunc.h"

namespace video {

Texture::Texture(TextureType type, const std::string& name, uint32_t empty) :
		io::IOResource(), _name(name), _type(type), _format(TextureFormat::RGBA) {
	glGenTextures(1, &_handle);
	upload(_format, (const uint8_t*)&empty, 1, 1, 1);
	unbind();
	GL_checkError();
}

Texture::Texture(TextureType type, TextureFormat format, const std::string& name, const uint8_t* data, int width, int height, int index) :
		io::IOResource(), _name(name), _type(type), _format(format) {
	glGenTextures(1, &_handle);
	upload(_format, data, width, height, index);
	unbind();
	GL_checkError();
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

void Texture::upload(TextureFormat format, const uint8_t* data, int width, int height, int index) {
	_format = format;
	bind(0);
	const GLenum glformat = std::enum_value(_format);
	const GLenum gltype = std::enum_value(_type);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (_type == TextureType::Texture2D) {
		glTexImage2D(gltype, 0, (int)glformat, width, height, 0, glformat, GL_UNSIGNED_BYTE, (const void*)data);
//		glTexSubImage2D(gltype, 0, 0, 0, width, height, glformat, GL_UNSIGNED_BYTE, (const void*) data);
	} else {
		glTexSubImage3D(gltype, 0, 0, 0, 0, width, height, index, glformat, GL_UNSIGNED_BYTE, (const void*) data);
	}

	glTexParameteri(gltype, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(gltype, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(gltype, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(gltype, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(gltype, GL_TEXTURE_MAX_LEVEL, 0);
	_state = io::IOSTATE_LOADED;
	GL_checkError();
}

void Texture::bind(int unit) {
	if (unit != 0) {
		glActiveTexture(GL_TEXTURE0 + unit);
	}
	glBindTexture(std::enum_value(_type), _handle);
	GL_checkError();
	_boundUnit = unit;
	if (unit != 0) {
		glActiveTexture(GL_TEXTURE0);
	}
}

void Texture::unbind() {
	if (_boundUnit != 0) {
		glActiveTexture(GL_TEXTURE0 + _boundUnit);
	}
	glBindTexture(std::enum_value(_type), 0);
	GL_checkError();
	if (_boundUnit != 0) {
		glActiveTexture(GL_TEXTURE0);
	}
	_boundUnit = 0;
}

}
