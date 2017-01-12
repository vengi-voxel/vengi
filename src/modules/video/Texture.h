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

enum class TextureType {
	Texture2D = GL_TEXTURE_2D,
	Texture2DArray = GL_TEXTURE_2D_ARRAY
};

enum class TextureFormat {
	RGBA,
	RGB,
	D24S8
};

enum class TextureWrap {
	ClampToEdge = GL_CLAMP_TO_EDGE,
	Repeat = GL_REPEAT
};

class Texture: public io::IOResource {
private:
	std::string _name;
	GLuint _handle = 0u;
	int _width;
	int _height;
	TextureType _type;
	TextureFormat _format;
	TextureWrap _wrap;
	mutable TextureUnit _boundUnit = TextureUnit::Zero;

public:
	Texture(TextureType type, TextureFormat format, const std::string& name, int width = 1, int height = 1, int index = 1, TextureWrap wrap = TextureWrap::Repeat);
	~Texture();
	void shutdown();

	operator GLuint () const;
	TextureType type() const;
	TextureFormat format() const;
	TextureWrap wrap() const;
	int width() const;
	int height() const;
	GLuint handle() const;

	// updates the texture with the new data
	void upload(TextureFormat format, int width, int height, const uint8_t* data = nullptr, int index = 1);
	void upload(int width, int height, const uint8_t* data = nullptr, int index = 1);
	void upload(const uint8_t* data = nullptr, int index = 1);
	void bind(TextureUnit unit = TextureUnit::Zero) const;
	void unbind() const;
};

inline Texture::operator GLuint () const {
	return _handle;
}

inline TextureWrap Texture::wrap() const {
	return _wrap;
}

inline TextureFormat Texture::format() const {
	return _format;
}

inline int Texture::width() const {
	return _width;
}

inline int Texture::height() const {
	return _height;
}

inline TextureType Texture::type() const {
	return _type;
}

inline GLuint Texture::handle() const {
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

inline bool bindTexture(TextureUnit unit, const Texture& texture) {
	texture.bind(unit);
	return true;
}

}
