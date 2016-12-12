/**
 * @file
 */

#pragma once

#include "video/GLFunc.h"
#include "io/IOResource.h"
#include "image/Image.h"

#include <memory>

namespace video {

enum class TextureType {
	Texture2D = GL_TEXTURE_2D,
	Texture2DArray = GL_TEXTURE_2D_ARRAY
};

enum class TextureFormat {
	RGBA = GL_RGBA,
	RGB = GL_RGB
};

class Texture: public io::IOResource {
private:
	std::string _name;
	GLuint _handle;
	TextureType _type;
	TextureFormat _format;
	int _boundUnit = 0;

public:
	// creates an empty dummy texture with the given name
	Texture(TextureType type, const std::string& name, uint32_t empty = 0x00000000);
	// create a texture with the given name and uploads it
	Texture(TextureType type, TextureFormat format, const std::string& name, const uint8_t* data, int width, int height, int index = 1);
	~Texture();
	void shutdown();

	operator GLuint () const;
	TextureType type() const;
	GLuint handle() const;

	// updates the texture with the new data
	void upload(TextureFormat format, const uint8_t* data, int width, int height, int index = 1);
	void bind(int unit = 0);
	void unbind();
};

inline Texture::operator GLuint () const {
	return _handle;
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
	return TexturePtr(new Texture(TextureType::Texture2D, name));
}

// creates white texture with placeholder pixel in
inline TexturePtr createWhiteTexture(const std::string& name) {
	return TexturePtr(new Texture(TextureType::Texture2D, name, 0xFFFFFFFF));
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
	TexturePtr t(new Texture(TextureType::Texture2D, format, image->name(), image->data(), image->width(), image->height(), 1));
	return t;
}

inline TexturePtr createTextureFromImage(const std::string& filename) {
	return createTextureFromImage(image::loadImage(filename, false));
}

}
