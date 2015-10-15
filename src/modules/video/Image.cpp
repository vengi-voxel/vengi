#include "Image.h"
#include "core/Log.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include <stb_image.h>

namespace video {

Image::Image(const std::string& filename) :
		io::IOResource(), _filename(filename), _width(-1), _height(-1), _depth(-1), _data(nullptr) {
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
	if (!buffer || length <= 0) {
		_state = io::IOSTATE_FAILED;
		return;
	}
	_data = stbi_load_from_memory(buffer, length, &_width, &_height, &_depth, STBI_rgb_alpha);
	if (_data == nullptr)
		_state = io::IOSTATE_FAILED;
	else
		_state = io::IOSTATE_LOADED;
}

bool Image::loadSync() {
	loadAsync();
	while (isLoading()) {
	}
	return isLoaded();
}

void Image::loadAsync() {
	// TODO: push into io thread
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open(_filename);
	load(file);
}

}
