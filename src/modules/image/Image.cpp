/**
 * @file
 */

#include "Image.h"
#include "core/Log.h"
#include "app/App.h"
#include "core/StringUtil.h"
#include "core/concurrent/ThreadPool.h"
#include "core/Assert.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Filesystem.h"
#include "core/StandardLib.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "util/Base64.h"

#include <glm/common.hpp>
#include <glm/ext/scalar_common.hpp>

#define STBI_ASSERT core_assert
#define STBI_MALLOC core_malloc
#define STBI_REALLOC core_realloc
#define STBI_FREE core_free
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_HDR
#define STBI_NO_GIF

#define STBIW_ASSERT core_assert
#define STBIW_MALLOC core_malloc
#define STBIW_REALLOC core_realloc
#define STBIW_FREE core_free

#if __GNUC__
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough" // stb_image.h
#endif

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_IMPLEMENTATION

namespace image {

Image::Image(const core::String& name) :
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
	const bool status = load(buffer, length);
	delete[] buffer;
	return status;
}

const uint8_t* Image::at(int x, int y) const {
	core_assert_msg(x >= 0 && x < _width, "x out of bounds: x: %i, y: %i, w: %i, h: %i", x, y, _width, _height);
	core_assert_msg(y >= 0 && y < _height, "y out of bounds: x: %i, y: %i, w: %i, h: %i", x, y, _width, _height);
	const int colSpan = _width * _depth;
	const intptr_t offset = x * _depth + y * colSpan;
	return _data + offset;
}

core::RGBA Image::colorAt(int x, int y) const {
	const uint8_t *ptr = at(x, y);
	const int d = depth();
	if (d == 4) {
		return core::RGBA{ptr[0], ptr[1], ptr[2], ptr[3]};
	}
	if (d == 3) {
		return core::RGBA{ptr[0], ptr[1], ptr[2], 255};
	}
	core_assert(d == 1);
	return core::RGBA{ptr[0], ptr[0], ptr[0], 255};
}

core::RGBA Image::colorAt(const glm::vec2 &uv, TextureWrap wrapS, TextureWrap wrapT) const {
	const float w = (float)width();
	const float h = (float)height();
	float x;
	float y;
	switch (wrapS) {
	case TextureWrap::Repeat: {
		x = glm::repeat(uv.x) * (w - 1);
		break;
	}
	case TextureWrap::ClampToEdge: {
		x = glm::clamp(uv.x, 1.0f / (2.0f * w), 1.0f - 1.0f / (2.0f * w)) * (w - 1);
		break;
	}
	case TextureWrap::MirroredRepeat: {
		x = glm::mirrorRepeat(uv.x) * (w - 1);
		break;
	}
	default:
	case TextureWrap::Max:
		return 0;
	}
	switch (wrapT) {
	case TextureWrap::Repeat: {
		y = glm::repeat(uv.y) * (h - 1);
		break;
	}
	case TextureWrap::ClampToEdge: {
		y = glm::clamp(uv.y, 1.0f / (2.0f * h), 1.0f - 1.0f / (2.0f * h)) * (h - 1);
		break;
	}
	case TextureWrap::MirroredRepeat: {
		y = glm::mirrorRepeat(uv.y) * (h - 1);
		break;
	}
	default:
	case TextureWrap::Max:
		return 0;
	}

	const int xint = (int)x;
	const int yint = (int)y;
	return colorAt(xint, yint);
}

ImagePtr loadImage(const io::FilePtr& file, bool async) {
	const ImagePtr& i = createEmptyImage(file->name());
	if (async) {
		app::App::getInstance()->threadPool().enqueue([=] () { i->load(file); });
	} else {
		if (!i->load(file)) {
			Log::warn("Failed to load image %s", i->name().c_str());
		}
	}
	return i;
}

ImagePtr loadImage(const core::String& filename, bool async) {
	io::FilePtr file;
	if (!core::string::extractExtension(filename).empty()) {
		file = io::filesystem()->open(filename);
	} else {
		for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
			for (const core::String& ext : desc->exts) {
				const core::String &f = core::string::format("%s.%s", filename.c_str(), ext.c_str());
				if (io::filesystem()->exists(f)) {
					file = io::filesystem()->open(f);
					if (file) {
						break;
					}
				}
			}
			if (file) {
				break;
			}
		}
	}
	if (!file || !file->validHandle()) {
		return createEmptyImage(filename);
	}
	return loadImage(file, async);
}

bool Image::load(const uint8_t* buffer, int length) {
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

bool Image::loadRGBA(const uint8_t* buffer, int width, int height) {
	const int length = width * height * 4;
	if (!buffer || length <= 0) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: buffer empty", _name.c_str());
		return false;
	}
	if (_data) {
		stbi_image_free(_data);
	}
	_data = (uint8_t*)STBI_MALLOC(length);
	_width = width;
	_height = height;
	core_memcpy(_data, buffer, length);
	// we are always using rgba
	_depth = 4;
	Log::debug("Loaded image %s", _name.c_str());
	_state = io::IOSTATE_LOADED;
	return true;
}

void Image::flipVerticalRGBA(uint8_t *pixels, int w, int h) {
	uint32_t *srcPtr = reinterpret_cast<uint32_t *>(pixels);
	uint32_t *dstPtr = srcPtr + (((intptr_t)h - (intptr_t)1) * (intptr_t)w);
	for (int y = 0; y < h / 2; ++y) {
		for (int x = 0; x < w; ++x) {
			const uint32_t d = dstPtr[x];
			const uint32_t s = srcPtr[x];
			dstPtr[x] = s;
			srcPtr[x] = d;
		}
		srcPtr += w;
		dstPtr -= w;
	}
}

glm::vec2 Image::uv(int x, int y) const {
	return glm::vec2((float)x / (float)_width, (float)y / (float)_height);
}

bool Image::writePng(const char *name, const uint8_t* buffer, int width, int height, int depth) {
	return stbi_write_png(name, width, height, depth, (const void*)buffer, width * depth) != 0;
}

uint8_t* createPng(const void *pixels, int width, int height, int depth, int *pngSize) {
	return (uint8_t*)stbi_write_png_to_mem((const unsigned char*)pixels, 0, width, height, depth, pngSize);
}

static void stream_write_func(void *context, void *data, int size) {
	io::SeekableWriteStream *stream = (io::SeekableWriteStream*)context;
	stream->write(data, size);
}

bool Image::writePng(io::SeekableWriteStream &stream, const uint8_t* buffer, int width, int height, int depth) {
	return stbi_write_png_to_func(stream_write_func, &stream, width, height, depth, (const void*)buffer, width * depth) != 0;
}

bool Image::writePng() const {
	if (_state != io::IOSTATE_LOADED) {
		return false;
	}
	return stbi_write_png(_name.c_str(), _width, _height, _depth, (const void*)_data, _width * _depth) != 0;
}

core::String Image::pngBase64() const {
	io::BufferedReadWriteStream s;
	if (!writePng(s, _data, _width, _height, _depth)) {
		return "";
	}
	s.seek(0);
	return util::Base64::encode(s);
}

}
