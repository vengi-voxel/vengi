/**
 * @file
 */

#include "Image.h"
#include "core/Log.h"
#include "core/App.h"
#include "io/Filesystem.h"

#define STBI_ASSERT core_assert
#define STBI_MALLOC core_malloc
#define STBI_REALLOC core_realloc
#define STBI_FREE core_free

#define STBIW_ASSERT core_assert
#define STBIW_MALLOC core_malloc
#define STBIW_REALLOC core_realloc
#define STBIW_FREE core_free

namespace image {

#if __GNUC__
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough" // stb_image.h
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Image::Image(const std::string& name) :
		io::IOResource(), _name(name) {
}

Image::~Image() {
	if (_data) {
		stbi_image_free(_data);
	}
}

bool Image::load(const io::FilePtr& file) {
	uint8_t* buffer;
	const int length = file->read((void**) &buffer);
	std::unique_ptr<uint8_t[]> p(buffer);
	return load(buffer, length);
}

ImagePtr loadImage(const io::FilePtr& file, bool async) {
	const ImagePtr& i = createEmptyImage(file->name());
	if (async) {
		core::App::getInstance()->threadPool().enqueue([=] () { i->load(file); });
	} else {
		if (!i->load(file)) {
			Log::warn("Failed to load image %s", i->name().c_str());
		}
	}
	return i;
}

ImagePtr loadImage(const std::string& filename, bool async) {
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open(filename);
	return loadImage(file, async);
}

bool Image::load(uint8_t* buffer, int length) {
	if (!buffer || length <= 0) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: buffer empty", _name.c_str());
		return false;
	}
	if (_data) {
		stbi_image_free(_data);
	}
	_data = stbi_load_from_memory(buffer, length, &_width, &_height, &_depth, STBI_rgb_alpha);
	// we are always using rgba
	_depth = 4;
	if (_data == nullptr) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: unsupported format", _name.c_str());
		return false;
	}
	Log::debug("Loaded image %s", _name.c_str());
	_state = io::IOSTATE_LOADED;
	return true;
}

bool Image::writePng(const char *name, const uint8_t* buffer, int width, int height, int depth) {
	return stbi_write_png(name, width, height, depth, (const void*)buffer, width * depth) != 0;
}

bool Image::writePng() const {
	if (_state != io::IOSTATE_LOADED) {
		return false;
	}
	return stbi_write_png(_name.c_str(), _width, _height, _depth, (const void*)_data, _width * _depth) != 0;
}

}
