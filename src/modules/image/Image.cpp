#include "Image.h"
#include "core/Log.h"
#include "core/App.h"
#include "io/Filesystem.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace video {

Image::Image(const std::string& name) :
		io::IOResource(), _name(name), _width(-1), _height(-1), _depth(-1), _data(nullptr) {
}

Image::~Image() {
	if (_data) {
		stbi_image_free(_data);
	}
}

void Image::load(const io::FilePtr& file) {
	uint8_t* buffer;
	const int length = file->read((void**) &buffer);
	std::unique_ptr<uint8_t[]> p(buffer);
	load(buffer, length);
}

void Image::load(uint8_t* buffer, int length) {
	if (!buffer || length <= 0) {
		_state = io::IOSTATE_FAILED;
		return;
	}
	if (_data) {
		stbi_image_free(_data);
	}
	_data = stbi_load_from_memory(buffer, length, &_width, &_height, &_depth, STBI_rgb_alpha);
	if (_data == nullptr)
		_state = io::IOSTATE_FAILED;
	else
		_state = io::IOSTATE_LOADED;
}

}
