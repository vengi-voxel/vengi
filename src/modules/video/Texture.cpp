/**
 * @file
 */

#include "Texture.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/String.h"
#include "image/Image.h"
#include "video/Renderer.h"

namespace video {

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

uint8_t* Texture::data() {
	if (_handle == InvalidId) {
		return nullptr;
	}
	uint8_t *pixels;
	if (!video::readTexture(TextureUnit::Upload, _config.type(), _config.format(), _handle, _width, _height, &pixels)) {
		return nullptr;
	}
	return pixels;
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
	_layerCount = core_max(_layerCount, index);
	_state = io::IOSTATE_LOADED;
}

void Texture::bind(TextureUnit unit) const {
	core_assert_always(_handle != InvalidId);
	video::bindTexture(unit, type(), _handle);
	_boundUnit = unit;
}

void Texture::unbind() const {
	video::bindTexture(_boundUnit, type(), InvalidId);
	_boundUnit = TextureUnit::Zero;
}

TexturePtr createTextureFromImage(const image::ImagePtr& image) {
	if (!image) {
		Log::warn("Could not load texture");
		return TexturePtr();
	}
	if (image->width() <= 0) {
		Log::warn("Could not load texture from image %s", image->name().c_str());
		return TexturePtr();
	}
	TextureConfig cfg;
	cfg.type(TextureType::Texture2D);
	if (image->depth() == 4) {
		cfg.format(TextureFormat::RGBA);
	} else {
		cfg.format(TextureFormat::RGB);
	}
	const TexturePtr& t = std::make_shared<Texture>(cfg, image->width(), image->height(), image->name());
	t->upload(image->data());
	return t;
}

TexturePtr createEmptyTexture(const std::string& name) {
	TextureConfig cfg;
	cfg.type(TextureType::Texture2D);
	cfg.format(TextureFormat::RGBA);
	const TexturePtr& p = std::make_shared<Texture>(cfg, 1, 1, name);
	const uint32_t empty = 0x00000000;
	p->upload((const uint8_t*)&empty);
	return p;
}

TexturePtr createWhiteTexture(const std::string& name) {
	TextureConfig cfg;
	cfg.type(TextureType::Texture2D);
	cfg.format(TextureFormat::RGBA);
	const TexturePtr& p = std::make_shared<Texture>(cfg, 1, 1, name);
	const uint32_t empty = 0xFFFFFFFF;
	p->upload((const uint8_t*)&empty);
	return p;
}

TexturePtr createTextureFromImage(const std::string& filename) {
	return createTextureFromImage(image::loadImage(filename, false));
}

TexturePtr createTexture(const TextureConfig& cfg, int width, int height, const std::string& name) {
	const TexturePtr& ptr = std::make_shared<Texture>(cfg, width, height, name);
	if (cfg.type() == TextureType::Texture2D && cfg.layers() > 1) {
		Log::error("Texture with layers given, but normal 2d texture was defined");
		return TexturePtr();
	}
	ptr->upload(nullptr, cfg.layers());
	return ptr;
}

bool bindTexture(TextureUnit unit, const Texture& texture) {
	texture.bind(unit);
	return true;
}

bool bindTexture(TextureUnit unit, const TexturePtr& texture) {
	if (!texture) {
		return false;
	}
	texture->bind(unit);
	return true;
}

}
