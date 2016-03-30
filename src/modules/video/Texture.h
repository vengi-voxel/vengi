#pragma once

#include "video/GLFunc.h"
#include "io/IOResource.h"
#include "video/Image.h"

#include <memory>

namespace video {

class Texture: public io::IOResource {
private:
	std::string _filename;
	GLuint _textureHandle;
	video::ImagePtr _img;

	void upload();
public:
	Texture(const std::string& filename);
	Texture(uint8_t* data, int width, int height, int depth);
	~Texture();

	void load();
	void bind();
	void unbind();
};

typedef std::shared_ptr<Texture> TexturePtr;

}
