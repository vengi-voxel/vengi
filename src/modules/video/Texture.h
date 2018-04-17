/**
 * @file
 */

#pragma once

#include "video/Renderer.h"
#include "video/Types.h"
#include "io/IOResource.h"
#include "image/Image.h"

#include <memory>

namespace video {

/**
 * @ingroup Video
 */
class Texture: public io::IOResource {
private:
	std::string _name;
	Id _handle = video::InvalidId;
	int _width;
	int _height;
	TextureConfig _config;
	mutable TextureUnit _boundUnit = TextureUnit::Zero;

public:
	Texture(TextureType type, TextureFormat format, const std::string& name, int width = 1, int height = 1, TextureWrap wrap = TextureWrap::Repeat, TextureFilter filter = TextureFilter::Linear);
	Texture(const TextureConfig& cfg, int width = 1, int height = 1, const std::string& name = "");
	~Texture();
	void shutdown();

	operator Id () const;
	TextureType type() const;
	TextureFormat format() const;
	int width() const;
	int height() const;
	Id handle() const;

	// updates the texture with the new data
	void upload(TextureFormat format, TextureFilter filter, int width, int height, const uint8_t* data = nullptr, int index = 1);
	void upload(TextureFormat format, int width, int height, const uint8_t* data = nullptr, int index = 1);
	void upload(int width, int height, const uint8_t* data = nullptr, int index = 1);
	void upload(const uint8_t* data = nullptr, int index = 1);
	/**
	 * @sa unbind()
	 */
	void bind(TextureUnit unit = TextureUnit::Zero) const;
	/**
	 * @sa bind()
	 */
	void unbind() const;
};

inline Texture::operator Id() const {
	return _handle;
}

inline TextureFormat Texture::format() const {
	return _config.format();
}

inline int Texture::width() const {
	return _width;
}

inline int Texture::height() const {
	return _height;
}

inline TextureType Texture::type() const {
	return _config.type();
}

inline Id Texture::handle() const {
	return _handle;
}

typedef std::shared_ptr<Texture> TexturePtr;

// creates empty texture with placeholder pixel in
inline TexturePtr createEmptyTexture(const std::string& name) {
	const TexturePtr& p = std::make_shared<Texture>(TextureType::Texture2D, TextureFormat::RGBA, name, 1, 1);
	const uint32_t empty = 0x00000000;
	p->upload((const uint8_t*)&empty);
	return p;
}

// creates white texture with placeholder pixel in
inline TexturePtr createWhiteTexture(const std::string& name) {
	const TexturePtr& p = std::make_shared<Texture>(TextureType::Texture2D, TextureFormat::RGBA, name, 1, 1);
	const uint32_t empty = 0xFFFFFFFF;
	p->upload((const uint8_t*)&empty);
	return p;
}

inline TexturePtr createTextureFromImage(const image::ImagePtr& image) {
	if (!image) {
		Log::warn("Could not load texture");
		return TexturePtr();
	}
	if (image->width() == -1) {
		Log::warn("Could not load texture from image %s", image->name().c_str());
		return TexturePtr();
	}
	TextureFormat format;
	if (image->depth() == 4) {
		format = TextureFormat::RGBA;
	} else {
		format = TextureFormat::RGB;
	}
	const TexturePtr& t = std::make_shared<Texture>(TextureType::Texture2D, format, image->name(), image->width(), image->height());
	t->upload(image->data());
	return t;
}

inline TexturePtr createTextureFromImage(const std::string& filename) {
	return createTextureFromImage(image::loadImage(filename, false));
}

inline TexturePtr createTexture(const TextureConfig& cfg, int width = 1, int height = 1, const std::string& name = "") {
	const TexturePtr& ptr = std::make_shared<Texture>(cfg, width, height, name);
	ptr->upload();
	return ptr;
}

inline bool bindTexture(TextureUnit unit, const Texture& texture) {
	texture.bind(unit);
	return true;
}

}
