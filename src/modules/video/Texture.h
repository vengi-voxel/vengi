/**
 * @file
 */

#pragma once

#include "video/GLFunc.h"
#include "io/IOResource.h"
#include "image/Image.h"

#include <memory>

namespace video {

class Texture: public io::IOResource {
private:
	std::string _name;
	GLuint _handle;
	int _boundUnit = 0;

public:
	// creates an empty dummy texture with the given name
	Texture(const std::string& name, uint32_t empty = 0x00000000);
	// create a texture with the given name and uploads it
	Texture(const std::string& name, const uint8_t* data, int width, int height, int depth);
	~Texture();
	void shutdown();

	// updates the texture with the new data
	void upload(const uint8_t* data, int width, int height, int depth);
	void bind(int unit = 0);
	void unbind();
};

typedef std::shared_ptr<Texture> TexturePtr;

// creates empty texture with placeholder pixel in
inline TexturePtr createEmptyTexture(const std::string& name) {
	return TexturePtr(new Texture(name));
}

// creates white texture with placeholder pixel in
inline TexturePtr createWhiteTexture(const std::string& name) {
	return TexturePtr(new Texture(name, 0xFFFFFFFF));
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
	TexturePtr t(new Texture(image->name(), image->data(), image->width(), image->height(), image->depth()));
	return t;
}

inline TexturePtr createTextureFromImage(const std::string& filename) {
	return createTextureFromImage(image::loadImage(filename, false));
}

}
