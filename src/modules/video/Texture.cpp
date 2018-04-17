/**
 * @file
 */

#include "Texture.h"
#include "core/Log.h"
#include "core/String.h"

namespace video {

Texture::Texture(TextureType type, TextureFormat format, const std::string& name, int width, int height, TextureWrap wrap, TextureFilter filter) :
		io::IOResource(), _name(name), _width(width), _height(height) {
	_config.type(type).wrap(wrap).filter(filter).format(format);

}

Texture::Texture(const TextureConfig& cfg, int width, int height, const std::string& name) :
		io::IOResource(), _name(name), _width(width), _height(height) {
	_config = cfg;
}

Texture::~Texture() {
	// in case of a texture we don't want this check, as it might be shared between multiple resources
	// and it should only be destroyed once it's completely destroyed by the shared_ptr
	//core_assert_msg(_handle == 0u, "Texture %s was not properly shut down", _name.c_str());
	shutdown();
}

void Texture::shutdown() {
	video::deleteTexture(_handle);
}

void Texture::upload(TextureFormat format, TextureFilter filter, int width, int height, const uint8_t* data, int index) {
	_config.filter(filter);
	upload(format, width, height, data, index);
}

void Texture::upload(TextureFormat format, int width, int height, const uint8_t* data, int index) {
	_config.format(format);
	upload(width, height, data, index);
}

void Texture::upload(const uint8_t* data, int index) {
	upload(_width, _height, data, index);
}

void Texture::upload(int width, int height, const uint8_t* data, int index) {
	if (_handle == InvalidId) {
		_handle = video::genTexture();
	}
	_width = width;
	_height = height;
	video::bindTexture(TextureUnit::Upload, type(), _handle);
	video::setupTexture(_config);
	video::uploadTexture(type(), format(), _width, _height, data, index);
	_state = io::IOSTATE_LOADED;
}

void Texture::bind(TextureUnit unit) const {
	video::bindTexture(unit, type(), _handle);
	_boundUnit = unit;
}

void Texture::unbind() const {
	video::bindTexture(_boundUnit, type(), InvalidId);
	_boundUnit = TextureUnit::Zero;
}

}
