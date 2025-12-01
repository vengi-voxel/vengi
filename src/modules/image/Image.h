/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"
#include "color/RGBA.h"
#include "core/SharedPtr.h"
#include "image/ImageType.h"
#include "io/File.h"
#include "io/IOResource.h"
#include "io/Stream.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>

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
class Image : public io::IOResource, core::NonCopyable {
private:
	core::String _name;
	int _width = -1;
	int _height = -1;
	int _colorComponents = -1;
	// the color data - either RGBA or RGB - depends on the color components
	uint8_t *_colors = nullptr;

public:
	Image(const core::String &name, int colorComponents = 4);
	~Image();

	/**
	 * @brief Loads the image by generating pixels using the provided function.
	 * @tparam FUNC A callable that takes (x, y, rgba) and fills the rgba color.
	 * @param w Width of the image to generate.
	 * @param h Height of the image to generate.
	 * @param func The function to generate pixel colors.
	 * @return true if the image was successfully generated, false otherwise.
	 */
	template<typename FUNC>
	bool load(int w, int h, FUNC &&func) {
		_colorComponents = 4;
		if (!resize(w, h)) {
			_state = io::IOSTATE_FAILED;
			return false;
		}
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				color::RGBA rgba;
				func(x, y, rgba);
				setColor(rgba, x, y);
			}
		}
		markLoaded();
		return true;
	}

	/**
	 * @brief Loads an image from a stream.
	 * @param type The type of the image (e.g., PNG, JPEG).
	 * @param stream The stream to read from.
	 * @param length The length of the data in the stream.
	 * @return true if the image was successfully loaded, false otherwise.
	 */
	bool load(ImageType type, io::SeekableReadStream &stream, int length);

	/**
	 * @brief Manually marks the image as loaded.
	 */
	void markLoaded();

	/**
	 * @brief Loads a raw RGBA buffer.
	 * @param buffer Pointer to the raw RGBA data.
	 * @param width Width of the image.
	 * @param height Height of the image.
	 * @return true if the image was successfully loaded, false otherwise.
	 */
	bool loadRGBA(const uint8_t *buffer, int width, int height);

	/**
	 * @brief Loads raw RGBA data from a stream.
	 * @param stream The stream to read from.
	 * @param w Width of the image.
	 * @param h Height of the image.
	 * @return true if the image was successfully loaded, false otherwise.
	 */
	bool loadRGBA(io::ReadStream &stream, int w, int h);

	/**
	 * @brief Loads raw BGRA data from a stream and converts it to RGBA.
	 * @param stream The stream to read from.
	 * @param w Width of the image.
	 * @param h Height of the image.
	 * @return true if the image was successfully loaded, false otherwise.
	 */
	bool loadBGRA(io::ReadStream &stream, int w, int h);

	/**
	 * @brief Sets the alpha channel of all pixels to 255 (opaque).
	 */
	void makeOpaque();

	/**
	 * @brief Converts UV coordinates to pixel coordinates.
	 * @param uv The UV coordinates.
	 * @param w Width of the texture.
	 * @param h Height of the texture.
	 * @param wrapS Wrapping mode for the S coordinate.
	 * @param wrapT Wrapping mode for the T coordinate.
	 * @param originUpperLeft If true, origin is top-left; otherwise, bottom-left.
	 */
	static glm::ivec2 pixels(const glm::vec2 &uv, int w, int h, TextureWrap wrapS = TextureWrap::Repeat,
							 TextureWrap wrapT = TextureWrap::Repeat, bool originUpperLeft = false);

	/**
	 * @brief Converts UV coordinates to pixel coordinates using the current image dimensions.
	 * @param uv The UV coordinates.
	 * @param wrapS Wrapping mode for the S coordinate.
	 * @param wrapT Wrapping mode for the T coordinate.
	 * @param originUpperLeft If true, origin is top-left; otherwise, bottom-left.
	 */
	glm::ivec2 pixels(const glm::vec2 &uv, TextureWrap wrapS = TextureWrap::Repeat,
					  TextureWrap wrapT = TextureWrap::Repeat, bool originUpperLeft = false) const;

	/**
	 * @brief Converts pixel coordinates to UV coordinates.
	 * @sa MeshFormat::paletteUV()
	 * @param x X pixel coordinate.
	 * @param y Y pixel coordinate.
	 * @param originUpperLeft If true, origin is top-left; otherwise, bottom-left.
	 */
	glm::vec2 uv(int x, int y, bool originUpperLeft = false) const;

	/**
	 * @brief Converts pixel coordinates to UV coordinates.
	 * @param x X pixel coordinate.
	 * @param y Y pixel coordinate.
	 * @param w Width of the image.
	 * @param h Height of the image.
	 * @param originUpperLeft If true, origin is top-left; otherwise, bottom-left.
	 */
	static glm::vec2 uv(int x, int y, int w, int h, bool originUpperLeft = false);

	/**
	 * @brief Resizes the image to the specified dimensions.
	 * @param w New width.
	 * @param h New height.
	 * @return true if the resize was successful, false otherwise.
	 */
	bool resize(int w, int h);

	/**
	 * @brief Flips the image vertically in place.
	 * @param pixels Pointer to the raw pixel data.
	 * @param w Width of the image.
	 * @param h Height of the image.
	 */
	static void flipVerticalRGBA(uint8_t *pixels, int w, int h);

	/**
	 * @brief Writes the image as PNG to a stream.
	 * @param stream The stream to write to.
	 * @return true if successful, false otherwise.
	 */
	bool writePNG(io::SeekableWriteStream &stream) const;

	/**
	 * @brief Writes a raw buffer as PNG to a stream.
	 * @param stream The stream to write to.
	 * @param buffer The raw pixel data.
	 * @param width Width of the image.
	 * @param height Height of the image.
	 * @param components Number of color components.
	 * @return true if successful, false otherwise.
	 */
	static bool writePNG(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components);

	/**
	 * @brief Writes a raw buffer as JPEG to a stream.
	 * @param stream The stream to write to.
	 * @param buffer The raw pixel data.
	 * @param width Width of the image.
	 * @param height Height of the image.
	 * @param components Number of color components.
	 * @param quality JPEG quality (1-100).
	 * @return true if successful, false otherwise.
	 */
	static bool writeJPEG(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components,
						  int quality = 100);

	/**
	 * @brief Writes the image as JPEG to a stream.
	 * @param stream The stream to write to.
	 * @param quality JPEG quality (1-100).
	 * @return true if successful, false otherwise.
	 */
	bool writeJPEG(io::SeekableWriteStream &stream, int quality = 100) const;

	/**
	 * @brief Returns the image as a Base64 encoded PNG string.
	 */
	core::String pngBase64() const;

	/**
	 * @brief Returns the color at the specified pixel coordinates.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 */
	color::RGBA colorAt(int x, int y) const;

	/**
	 * @brief Returns the color at the specified UV coordinates.
	 * @param uv UV coordinates.
	 * @param wrapS Wrapping mode for S.
	 * @param wrapT Wrapping mode for T.
	 * @param originUpperLeft If true, origin is top-left; otherwise, bottom-left.
	 */
	color::RGBA colorAt(const glm::vec2 &uv, TextureWrap wrapS = TextureWrap::Repeat,
					   TextureWrap wrapT = TextureWrap::Repeat, bool originUpperLeft = false) const;

	/**
	 * @return true if the image is grayscale, false otherwise.
	 */
	bool isGrayScale() const;

	/**
	 * @brief Sets the color at the specified pixel coordinates.
	 * @param rgba The color to set.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @return true if successful, false if coordinates are out of bounds.
	 */
	bool setColor(color::RGBA rgba, int x, int y);

	/**
	 * @brief Returns a pointer to the raw pixel data at the specified coordinates.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 */
	const uint8_t *at(int x, int y) const;

	/**
	 * @brief Sets the name of the image.
	 * @param name The new name.
	 */
	void setName(const core::String &name);

	/**
	 * @brief Returns the name of the image.
	 */
	const core::String &name() const;

	/**
	 * @brief Returns a pointer to the raw image data.
	 */
	const uint8_t *data() const;

	/**
	 * @brief Returns the dimensions of the image.
	 */
	glm::vec2 size() const;

	/**
	 * @brief Returns the width of the image.
	 */
	int width() const;

	/**
	 * @brief Returns the height of the image.
	 */
	int height() const;

	/**
	 * @brief Returns the number of color components.
	 */
	int components() const;

	/**
	 * @brief Returns the aspect ratio of the image.
	 * @return The aspect ratio (width / height).
	 */
	float aspect() const;
};

inline void Image::markLoaded() {
	_state = io::IOSTATE_LOADED;
}

inline void Image::setName(const core::String &name) {
	_name = name;
}

inline const core::String &Image::name() const {
	return _name;
}

inline const uint8_t *Image::data() const {
	return _colors;
}

inline glm::vec2 Image::size() const {
	return {_width, _height};
}

inline int Image::width() const {
	return _width;
}

inline int Image::height() const {
	return _height;
}

inline int Image::components() const {
	return _colorComponents;
}

inline float Image::aspect() const {
	return (float)_width / (float)_height;
}

typedef core::SharedPtr<Image> ImagePtr;

// creates an empty image
inline ImagePtr createEmptyImage(const core::String &name) {
	return core::make_shared<Image>(name);
}

ImagePtr loadImage(const io::FilePtr &file);
ImagePtr loadImage(const core::String &name, io::SeekableReadStream &stream, int length = -1);
ImagePtr loadRGBAImageFromStream(const core::String &name, io::ReadStream &stream, int w, int h);

/**
 * @brief If there is no extension given, all supported extensions are tried
 */
ImagePtr loadImage(const core::String &filename);

bool writePNG(const image::ImagePtr &image, io::SeekableWriteStream &stream);
core::String print(const image::ImagePtr &image, bool limited = true);

} // namespace image
