/**
 * @file
 */

#include "Image.h"
#include "app/App.h"
#include "color/ColorUtil.h"
#include "core/Assert.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/collection/BufferView.h"
#include "image/ImageType.h"
#include "image/private/JPEG.h"
#include "image/private/PNG.h"
#include "image/private/StbImage.h"
#include "io/Base64.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"

#include <glm/common.hpp>
#include <glm/ext/scalar_common.hpp>

#include <stb_image_resize2.h>

namespace image {

Image::Image(const core::String &name, int colorComponents)
	: io::IOResource(), _name(name), _colorComponents(colorComponents) {
}

Image::~Image() {
	if (_colors) {
		core_free(_colors);
	}
}

bool Image::isGrayScale() const {
	if (_colorComponents == 1) {
		return true;
	}
	for (int x = 0; x < _width; ++x) {
		for (int y = 0; y < _height; ++y) {
			const color::RGBA c = colorAt(x, y);
			if (c.r != c.g || c.r != c.b) {
				return false;
			}
		}
	}
	return true;
}

const uint8_t *Image::at(int x, int y) const {
	core_assert_msg(x >= 0 && x < _width, "x out of bounds: x: %i, y: %i, w: %i, h: %i", x, y, _width, _height);
	core_assert_msg(y >= 0 && y < _height, "y out of bounds: x: %i, y: %i, w: %i, h: %i", x, y, _width, _height);
	const int colSpan = _width * _colorComponents;
	const intptr_t offset = x * _colorComponents + y * colSpan;
	return _colors + offset;
}

bool Image::setColor(color::RGBA rgba, int x, int y) {
	if (x < 0 || x >= _width) {
		return false;
	}
	if (y < 0 || y >= _height) {
		return false;
	}
	if (_colorComponents != 4) {
		Log::error("Failed to set rgba color for an image with %i components", _colorComponents);
		return false;
	}
	const int colSpan = _width * _colorComponents;
	const intptr_t offset = x * _colorComponents + y * colSpan;
	uint8_t* pixel = _colors + offset;
	pixel[0] = rgba.r;
	pixel[1] = rgba.g;
	pixel[2] = rgba.b;
	pixel[3] = rgba.a;
	return true;
}

color::RGBA Image::colorAt(int x, int y) const {
	const uint8_t *ptr = at(x, y);
	const int d = components();
	if (d == 4) {
		return color::RGBA{ptr[0], ptr[1], ptr[2], ptr[3]};
	}
	if (d == 3) {
		return color::RGBA{ptr[0], ptr[1], ptr[2], 255};
	}
	core_assert(d == 1);
	return color::RGBA{ptr[0], ptr[0], ptr[0], 255};
}

color::RGBA Image::colorAt(const glm::vec2 &uv, TextureWrap wrapS, TextureWrap wrapT, bool originUpperLeft) const {
	const glm::ivec2 pc = pixels(uv, wrapS, wrapT, originUpperLeft);
	return colorAt(pc.x, pc.y);
}

bool writePNG(const image::ImagePtr &image, io::SeekableWriteStream &stream) {
	if (!image) {
		return false;
	}
	return image->writePNG(stream);
}

/**
 * @brief Detect image type by magic bytes
 */
static ImageType getImageType(io::SeekableReadStream &stream) {
	const int64_t pos = stream.pos();
	uint8_t header[4];
	if (stream.read(header, sizeof(header)) == -1) {
		stream.seek(pos, SEEK_SET);
		return ImageType::Unknown;
	}
	if (stream.seek(pos, SEEK_SET) == -1) {
		return ImageType::Unknown;
	}
	// 137,80,78,71,13,10,26,10
	if (header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G') {
		return ImageType::PNG;
	}
	if (header[0] == 0xFF && header[1] == 0xD8 /*&& header[2] == 0xFF && (header[3] == 0xE0 || header[3] == 0xE1)*/) {
		// 3rd and 4th byte are not always reliable and can be application specific
		// JFIF = 0xE0 and EXIF = 0xE1
		return ImageType::JPEG;
	}
	if (header[0] == 'G' && header[1] == 'I' && header[2] == 'F' && header[3] == '8') {
		return ImageType::GIF;
	}
	return ImageType::Unknown;
}

/**
 * @brief Detect image type by file extension
 */
static ImageType getImageType(const core::String &filename) {
	const struct Mapping {
		const char *ext;
		ImageType type;
	} mappings[] = {
		// clang-format off
		{"png", ImageType::PNG},
		{"jpg", ImageType::JPEG},
		{"jpeg", ImageType::JPEG},
		{"tga", ImageType::TGA},
		{"dds", ImageType::DDS},
		{"pkm", ImageType::PKM},
		{"pvr", ImageType::PVR},
		{"bmp", ImageType::BMP},
		{"psd", ImageType::PSD},
		{"gif", ImageType::GIF},
		{"hdr", ImageType::HDR},
		{"pic", ImageType::PICT},
		{"pnm", ImageType::PNM}
		// clang-format on
	};
	static_assert(lengthof(mappings) == (int)ImageType::Max, "ImageType mapping mismatch");
	const core::String ext = core::string::extractExtension(filename).toLower();
	for (const Mapping &m : mappings) {
		if (ext == m.ext) {
			return m.type;
		}
	}
	return ImageType::Unknown;
}

bool writePNG(const image::ImagePtr &image, const core::String &filename) {
	if (!image) {
		return false;
	}
	if (!image->isLoaded()) {
		return false;
	}
	if (filename.empty()) {
		return false;
	}
	const io::FilePtr &file = io::filesystem()->open(filename, io::FileMode::SysWrite);
	if (!file->validHandle()) {
		return false;
	}
	io::FileStream stream(file);
	ImageType type = getImageType(filename);
	if (type == ImageType::JPEG) {
		return image->writeJPEG(stream);
	} else if (type == ImageType::PNG) {
		return image->writePNG(stream);
	}
	Log::warn("Failed to write image %s: unsupported format", filename.c_str());
	return false;
}

ImagePtr loadImage(const io::FilePtr &file) {
	const ImagePtr &i = createEmptyImage(file->name());
	file->seek(0, SEEK_SET);
	io::FileStream stream(file);
	ImageType type = getImageType(file->name());
	if (!i->load(type, stream, stream.size())) {
		Log::warn("Failed to load image %s", i->name().c_str());
	}
	return i;
}

ImagePtr loadImage(const core::String &name, io::SeekableReadStream &stream, int length) {
	const ImagePtr &i = createEmptyImage(name);
	ImageType type = getImageType(name);
	if (!i->load(type, stream, length <= 0 ? (int)stream.size() : length)) {
		Log::warn("Failed to load image %s", i->name().c_str());
	}
	return i;
}

ImagePtr loadRGBAImageFromStream(const core::String &name, io::ReadStream &stream, int w, int h) {
	const ImagePtr &i = createEmptyImage(name);
	if (!i->loadRGBA(stream, w, h)) {
		Log::warn("Failed to load image %s", i->name().c_str());
	}
	return i;
}

ImagePtr loadImage(const core::String &filename) {
	io::FilePtr file;
	if (!core::string::extractExtension(filename).empty()) {
		file = io::filesystem()->open(filename);
	} else {
		for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
			for (const core::String &ext : desc->exts) {
				const core::String &f = core::String::format("%s.%s", filename.c_str(), ext.c_str());
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
		Log::debug("Could not open image '%s'", filename.c_str());
		return createEmptyImage(filename);
	}
	Log::debug("Load image '%s'", filename.c_str());
	return loadImage(file);
}

bool Image::load(ImageType type, io::SeekableReadStream &stream, int length) {
	if (length <= 0) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: buffer stream", _name.c_str());
		return false;
	}
	if (_colors) {
		core_free(_colors);
	}
	if (type == ImageType::Unknown) {
		type = getImageType(_name);
	}
	if (type == ImageType::Unknown) {
		type = getImageType(stream);
	}
	bool success = false;
	if (type == ImageType::JPEG) {
		success = format::JPEG::load(stream, length, _width, _height, _colorComponents, &_colors);
	} else if (type == ImageType::PNG) {
		success = format::PNG::load(stream, length, _width, _height, _colorComponents, &_colors);
	} else {
		success = format::StbImage::load(stream, length, _width, _height, _colorComponents, &_colors);
	}
	if (!success) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: unsupported format: %s", _name.c_str(), stbi_failure_reason());
		return false;
	}
	if (_colors == nullptr) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: unsupported format: %s", _name.c_str(), stbi_failure_reason());
		return false;
	}
	Log::debug("Loaded image %s", _name.c_str());
	_state = io::IOSTATE_LOADED;
	return true;
}

bool Image::loadRGBA(const uint8_t *buffer, int width, int height) {
	const int length = width * height * 4;
	io::MemoryReadStream stream(buffer, length);
	return loadRGBA(stream, width, height);
}

bool Image::loadBGRA(io::ReadStream &stream, int w, int h) {
	if (!loadRGBA(stream, w, h)) {
		return false;
	}
	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			color::RGBA rgba = colorAt(x, y);
			setColor(color::RGBA(rgba.b, rgba.g, rgba.r, rgba.a), x, y);
		}
	}
	return true;
}

bool Image::loadRGBA(io::ReadStream &stream, int w, int h) {
	const int length = w * h * 4;
	if (length <= 0) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: invalid size", _name.c_str());
		return false;
	}
	if (_colors) {
		core_free(_colors);
	}
	_colors = (uint8_t *)core_malloc(length);
	_width = w;
	_height = h;
	if (stream.read(_colors, length) != length) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: failed to read from stream", _name.c_str());
		return false;
	}
	// we are always using rgba
	_colorComponents = 4;
	Log::debug("Loaded image %s", _name.c_str());
	_state = io::IOSTATE_LOADED;
	return true;
}

void Image::makeOpaque() {
	if (_colorComponents != 4) {
		return;
	}
	for (int x = 0; x < _width; ++x) {
		for (int y = 0; y < _height; ++y) {
			color::RGBA rgba = colorAt(x, y);
			rgba.a = 255;
			setColor(rgba, x, y);
		}
	}
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

// OpenGL Spec 14.8.2 Coordinate Wrapping and Texel Selection
glm::ivec2 Image::pixels(const glm::vec2 &uv, TextureWrap wrapS, TextureWrap wrapT, bool originUpperLeft) const {
	const int w = width();
	const int h = height();
	return pixels(uv, w, h, wrapS, wrapT, originUpperLeft);
}

glm::ivec2 Image::pixels(const glm::vec2 &uv, int w, int h, TextureWrap wrapS, TextureWrap wrapT,
						 bool originUpperLeft) {
	int xint = (int)glm::round(uv.x * (w - 1));
	int yint = (int)glm::round(uv.y * (h - 1));
	if (!originUpperLeft) {
		yint = h - 1 - yint;
	}
	switch (wrapS) {
	case TextureWrap::Repeat:
		xint %= w;
		break;
	case TextureWrap::MirroredRepeat:
		xint = glm::abs(xint) % w;
		break;
	case TextureWrap::ClampToEdge:
		xint = glm::clamp(xint, 0, w - 1);
		break;
	case TextureWrap::Max:
		return glm::ivec2(0);
	}
	switch (wrapT) {
	case TextureWrap::Repeat:
		yint %= h;
		break;
	case TextureWrap::MirroredRepeat:
		yint = glm::abs(yint) % h;
		break;
	case TextureWrap::ClampToEdge:
		yint = glm::clamp(yint, 0, h - 1);
		break;
	case TextureWrap::Max:
		return glm::ivec2(0);
	}

	while (xint < 0) {
		xint = glm::abs(xint) % w;
		xint = w - xint;
	}

	while (yint < 0) {
		yint = glm::abs(yint) % h;
		yint = h - yint;
	}

	if (xint >= w) {
		xint %= w;
	}

	if (yint >= h) {
		yint %= h;
	}
	return glm::ivec2(xint, yint);
}

glm::vec2 Image::uv(int x, int y, bool originUpperLeft) const {
	return uv(x, y, _width, _height, originUpperLeft);
}

glm::vec2 Image::uv(int x, int y, int w, int h, bool originUpperLeft) {
	float u = 0.0f;
	if (w > 1) {
		u = ((float)x) / (float)(w - 1);
	}
	float v = 0.0f;
	if (h > 1) {
		if (originUpperLeft) {
			v = ((float)y) / (float)h;
		} else {
			v = ((float)h - 1.0f - (float)y) / (float)(h - 1);
		}
	}
	return glm::vec2(u, v);
}

bool Image::resize(int w, int h) {
	if (_width == w && _height == h) {
		return true;
	}
	uint8_t *res = (uint8_t *)core_malloc(w * h * _colorComponents);
	if (_colors) {
		if (stbir_resize(_colors, _width, _height, _colorComponents * _width, res, w, h, _colorComponents * w,
						 (stbir_pixel_layout)_colorComponents, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP,
						 STBIR_FILTER_DEFAULT) == nullptr) {
			core_free(res);
			return false;
		}
		core_free(_colors);
	}
	_colors = res;
	_width = w;
	_height = h;
	return true;
}

bool Image::writePNG(io::SeekableWriteStream &stream) const {
	return writePNG(stream, _colors, _width, _height, _colorComponents);
}

bool Image::writeJPEG(io::SeekableWriteStream &stream, int quality) const {
	return writeJPEG(stream, _colors, _width, _height, _colorComponents, quality);
}

bool Image::writeJPEG(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components,
					  int quality) {
	return image::format::JPEG::write(stream, buffer, width, height, components, quality);
}

bool Image::writePNG(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components) {
	return image::format::PNG::write(stream, buffer, width, height, components);
}

core::String Image::pngBase64() const {
	io::BufferedReadWriteStream s(_width * _height * _colorComponents);
	if (!writePNG(s, _colors, _width, _height, _colorComponents)) {
		return core::String::Empty;
	}
	s.seek(0);
	return io::Base64::encode(s);
}

core::String print(const image::ImagePtr &image, bool limited) {
	if (!image || !image->isLoaded()) {
		return "Image not loaded";
	}
	core::String str =
		core::String::format("w: %i, h: %i, d: %i\n", image->width(), image->height(), image->components());
	const int maxWidth = limited ? 64 : image->width();
	const int maxHeight = limited ? 64 : image->height();
	const int width = glm::min(image->width(), maxWidth);
	const int height = glm::min(image->height(), maxHeight);
	str.reserve(40 * width * height);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const color::RGBA color = image->colorAt(x, y);
			str.append(color::print(color, false));
		}
		str.append("\n");
	}
	return str;
}

} // namespace image
