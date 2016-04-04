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

public:
	// creates an empty dummy texture with the given name
	Texture(const std::string& name);
	// create a texture with the given name and uploads it
	Texture(const std::string& name, const uint8_t* data, int width, int height, int depth);
	~Texture();

	// updates the texture with the new data
	void upload(const uint8_t* data, int width, int height, int depth);
	void bind();
	void unbind();
};

typedef std::shared_ptr<Texture> TexturePtr;

// creates empty texture with placeholder pixel in
inline TexturePtr createTexture(const std::string& name) {
	return TexturePtr(new Texture(name));
}

inline TexturePtr createTextureFromImage(const ImagePtr& image) {
	if (!image) {
		return TexturePtr();
	}
	TexturePtr t(new Texture(image->name(), image->data(), image->width(), image->height(), image->depth()));
	return t;
}

inline TexturePtr createTextureFromImage(const std::string& filename) {
	return createTextureFromImage(createImage(filename));
}

}
