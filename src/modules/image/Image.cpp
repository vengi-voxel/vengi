/**
 * @file
 */

#include "Image.h"
#include "app/App.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/collection/BufferView.h"
#include "image/ImageType.h"
#include "io/Base64.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"

#include <glm/common.hpp>
#include <glm/ext/scalar_common.hpp>

#ifdef USE_LIBJPEG
#include <jpeglib.h>
#endif

#define STBI_NO_STDIO
#include <stb_image.h>

#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>

#include <stb_image_resize2.h>

namespace image {

Image::Image(const core::String &name) : io::IOResource(), _name(name) {
}

Image::~Image() {
	if (_data) {
		stbi_image_free(_data);
	}
}

bool Image::isGrayScale() const {
	if (_depthOfColor == 1) {
		return true;
	}
	for (int x = 0; x < _width; ++x) {
		for (int y = 0; y < _height; ++y) {
			const core::RGBA c = colorAt(x, y);
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
	const int colSpan = _width * _depthOfColor;
	const intptr_t offset = x * _depthOfColor + y * colSpan;
	return _data + offset;
}

void Image::setColor(core::RGBA rgba, int x, int y) {
	if (x < 0 || x >= _width) {
		return;
	}
	if (y < 0 || y >= _height) {
		return;
	}
	const int colSpan = _width * _depthOfColor;
	const intptr_t offset = x * _depthOfColor + y * colSpan;
	*(core::RGBA *)(_data + offset) = rgba;
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

core::RGBA Image::colorAt(const glm::vec2 &uv, TextureWrap wrapS, TextureWrap wrapT, bool originUpperLeft) const {
	const glm::ivec2 pc = pixels(uv, wrapS, wrapT, originUpperLeft);
	return colorAt(pc.x, pc.y);
}

bool writeImage(const image::Image &image, io::SeekableWriteStream &stream) {
	if (!image.isLoaded()) {
		return false;
	}
	return image.writePng(stream);
}

bool writeImage(const image::ImagePtr &image, io::SeekableWriteStream &stream) {
	if (!image)
		return false;
	return writeImage(*image.get(), stream);
}

static ImageType getImageType(const core::String &filename) {
	const core::String ext = core::string::extractExtension(filename).toLower();
	if (ext == "jpg" || ext == "jpeg") {
		return ImageType::JPEG;
	}
	return ImageType::PNG;
}

bool writeImage(const image::Image &image, const core::String &filename) {
	if (!image.isLoaded()) {
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
		return image.writeJPEG(stream);
	}
	return image.writePng(stream);
}

bool writeImage(const image::ImagePtr &image, const core::String &filename) {
	if (!image) {
		return false;
	}
	return writeImage(*image.get(), filename);
}

ImagePtr loadImage(const io::FilePtr &file) {
	const ImagePtr &i = createEmptyImage(file->name());
	io::FileStream stream(file);
	if (!i->load(stream, stream.size())) {
		Log::warn("Failed to load image %s", i->name().c_str());
	}
	return i;
}

ImagePtr loadImage(const core::String &name, io::SeekableReadStream &stream, int length) {
	const ImagePtr &i = createEmptyImage(name);
	if (!i->load(stream, length <= 0 ? (int)stream.size() : length)) {
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
		Log::debug("Could not open image '%s'", filename.c_str());
		return createEmptyImage(filename);
	}
	Log::debug("Load image '%s'", filename.c_str());
	return loadImage(file);
}

static int stream_read(void *user, char *data, int size) {
	io::ReadStream *stream = (io::ReadStream *)user;
	const int readSize = stream->read(data, size);
	// prevent endless loops on errors
	if (readSize < 0) {
		return 0;
	}
	return readSize;
}

static void stream_skip(void *user, int n) {
	io::ReadStream *stream = (io::ReadStream *)user;
	stream->skipDelta(n);
}

static int stream_eos(void *user) {
	io::ReadStream *stream = (io::ReadStream *)user;
	return stream->eos() ? 1 : 0;
}

bool Image::load(io::SeekableReadStream &stream, int length) {
	if (length <= 0) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: buffer empty", _name.c_str());
		return false;
	}
	if (_data) {
		stbi_image_free(_data);
	}
	stbi_io_callbacks clbk;
	clbk.read = stream_read;
	clbk.skip = stream_skip;
	clbk.eof = stream_eos;
	_data = stbi_load_from_callbacks(&clbk, &stream, &_width, &_height, &_depthOfColor, STBI_rgb_alpha);
	// we are always using rgba
	_depthOfColor = 4;
	if (_data == nullptr) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: unsupported format", _name.c_str());
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
			core::RGBA rgba = colorAt(x, y);
			setColor(core::RGBA(rgba.b, rgba.g, rgba.r, rgba.a), x, y);
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
	if (_data) {
		stbi_image_free(_data);
	}
	_data = (uint8_t *)core_malloc(length);
	_width = w;
	_height = h;
	if (stream.read(_data, length) != length) {
		_state = io::IOSTATE_FAILED;
		Log::debug("Failed to load image %s: failed to read from stream", _name.c_str());
		return false;
	}
	// we are always using rgba
	_depthOfColor = 4;
	Log::debug("Loaded image %s", _name.c_str());
	_state = io::IOSTATE_LOADED;
	return true;
}

void Image::makeOpaque() {
	if (_depthOfColor != 4) {
		return;
	}
	for (int x = 0; x < _width; ++x) {
		for (int y = 0; y < _height; ++y) {
			core::RGBA rgba = colorAt(x, y);
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
	uint8_t *res = (uint8_t *)core_malloc(w * h * _depthOfColor);
	if (_data) {
		if (stbir_resize(_data, _width, _height, _depthOfColor * _width, res, w, h, _depthOfColor * w,
						(stbir_pixel_layout)_depthOfColor, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP,
						STBIR_FILTER_DEFAULT) == nullptr) {
			core_free(res);
			return false;
		}
		stbi_image_free(_data);
	}
	_data = res;
	_width = w;
	_height = h;
	return true;
}

static void stream_write_func(void *context, void *data, int size) {
	io::SeekableWriteStream *stream = (io::SeekableWriteStream *)context;
	int64_t written = stream->write(data, size);
	if (written != size) {
		Log::error("Failed to write to image stream: %i vs %i", (int)written, size);
	}
}

bool Image::writePng(io::SeekableWriteStream &stream) const {
	return writePng(stream, _data, _width, _height, _depthOfColor);
}

bool Image::writeJPEG(io::SeekableWriteStream &stream, int quality) const {
	return writeJPEG(stream, _data, _width, _height, _depthOfColor, quality);
}

bool Image::writeJPEG(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int depth,
					  int quality) {
#ifdef USE_LIBJPEG
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *outbuffer = nullptr;
	unsigned long outsize = 0;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpeg_mem_dest(&cinfo, &outbuffer, &outsize);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = depth;
	if (depth == 1) {
		cinfo.in_color_space = JCS_GRAYSCALE;
	} else if (depth == 3) {
		cinfo.in_color_space = JCS_RGB;
	} else {
		cinfo.in_color_space = JCS_EXT_RGBA;
	}

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer[1];
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = (JSAMPROW)&buffer[cinfo.next_scanline * width * depth];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	stream.write(outbuffer, outsize);
	free(outbuffer);

	return true;
#else
	// stbi fallback implementation
	return stbi_write_jpg_to_func(stream_write_func, &stream, width, height, depth, buffer, quality) != 0;
#endif
}

bool Image::writePng(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int depth) {
	return stbi_write_png_to_func(stream_write_func, &stream, width, height, depth, (const void *)buffer,
								  width * depth) != 0;
}

core::String Image::pngBase64() const {
	io::BufferedReadWriteStream s;
	if (!writePng(s, _data, _width, _height, _depthOfColor)) {
		return "";
	}
	s.seek(0);
	return io::Base64::encode(s);
}

core::String print(const image::ImagePtr &image, bool limited) {
	if (!image || !image->isLoaded()) {
		return "Image not loaded";
	}
	core::String str = core::string::format("w: %i, h: %i, d: %i\n", image->width(), image->height(), image->depth());
	const int maxWidth = limited ? 64 : image->width();
	const int maxHeight = limited ? 64 : image->height();
	const int width = glm::min(image->width(), maxWidth);
	const int height = glm::min(image->height(), maxHeight);
	str.reserve(40 * width * height);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const core::RGBA color = image->colorAt(x, y);
			str.append(core::Color::print(color, false));
		}
		str.append("\n");
	}
	return str;
}

} // namespace image
