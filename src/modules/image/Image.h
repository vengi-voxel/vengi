/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
#include "io/IOResource.h"
#include "io/File.h"
#include "core/SharedPtr.h"
#include "io/Stream.h"
#include <glm/fwd.hpp>

namespace image {

// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
enum TextureWrap : uint8_t {
	Repeat, // causes the integer part of the s coordinate to be ignored; the GL uses only the fractional part, thereby
			// creating a repeating pattern.
	ClampToEdge, // causes s coordinates to be clamped to the range [1/2N,1−1/2N], where N is the size of the texture in
				 // the direction of clamping.
	MirroredRepeat, // causes the s coordinate to be set to the fractional part of the texture coordinate if the integer
					// part of s is even; if the integer part of s is odd, then the s texture coordinate is set to
					// 1−frac(s), where frac(s) represents the fractional part of s

	Max
};

/**
 * @brief Wrapper for image loading
 */
class Image: public io::IOResource {
private:
	core::String _name;
	int _width = -1;
	int _height = -1;
	int _depth = -1;
	uint8_t* _data = nullptr;

public:
	Image(const core::String& name);
	~Image();

	bool load(const io::FilePtr& file);
	bool load(const uint8_t* buffer, int length);
	bool load(io::SeekableReadStream &stream, int length);
	/**
	 * Loads a raw RGBA buffer
	 */
	bool loadRGBA(const uint8_t* buffer, int width, int height);
	bool loadRGBA(io::ReadStream& stream, int w, int h);
	bool loadBGRA(io::ReadStream& stream, int w, int h);

	glm::ivec2 pixels(const glm::vec2 &uv, TextureWrap wrapS = TextureWrap::Repeat, TextureWrap wrapT = TextureWrap::Repeat) const;
	glm::vec2 uv(int x, int y) const;
	static glm::vec2 uv(int x, int y, int w, int h);

	static void flipVerticalRGBA(uint8_t *pixels, int w, int h);
	bool writePng(io::SeekableWriteStream &stream) const;
	static bool writePng(io::SeekableWriteStream &stream, const uint8_t* buffer, int width, int height, int depth);
	static bool writePng(const char *name, const uint8_t *buffer, int width, int height, int depth);
	bool writePng() const;
	core::String pngBase64() const;
	core::RGBA colorAt(int x, int y) const;
	core::RGBA colorAt(const glm::vec2 &uv, TextureWrap wrapS = TextureWrap::Repeat,
					   TextureWrap wrapT = TextureWrap::Repeat) const;

	void setColor(core::RGBA rgba, int x, int y);

	const uint8_t* at(int x, int y) const;

	inline const core::String& name() const {
		return _name;
	}

	inline const uint8_t* data() const {
		return _data;
	}

	inline int width() const {
		return _width;
	}

	inline int height() const {
		return _height;
	}

	inline int depth() const {
		return _depth;
	}
};

typedef core::SharedPtr<Image> ImagePtr;

// creates an empty image
inline ImagePtr createEmptyImage(const core::String& name) {
	return core::make_shared<Image>(name);
}

extern uint8_t* createPng(const void *pixels, int width, int height, int depth, int *pngSize);
extern ImagePtr loadImage(const io::FilePtr& file, bool async = true);
extern ImagePtr loadImage(const core::String& filename, bool async = true);

extern core::String print(const image::ImagePtr &image);

}
