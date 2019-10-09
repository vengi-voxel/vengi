/**
 * @file
 */

#pragma once

#include "core/io/IOResource.h"
#include "core/io/File.h"
#include <memory>

namespace image {

/**
 * @brief Wrapper for image loading
 */
class Image: public io::IOResource {
private:
	std::string _name;
	int _width = -1;
	int _height = -1;
	int _depth = -1;
	uint8_t* _data = nullptr;

public:
	Image(const std::string& name);
	~Image();

	bool load(const io::FilePtr& file);
	bool load(const uint8_t* buffer, int length);

	static void flipVerticalRGBA(uint8_t *pixels, int w, int h);
	static bool writePng(const char *name, const uint8_t *buffer, int width, int height, int depth);
	bool writePng() const;

	const uint8_t* at(int x, int y) const;

	inline const std::string& name() const {
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

typedef std::shared_ptr<Image> ImagePtr;

// creates an empty image
inline ImagePtr createEmptyImage(const std::string& name) {
	return std::make_shared<Image>(name);
}

extern ImagePtr loadImage(const io::FilePtr& file, bool async = true);
extern ImagePtr loadImage(const std::string& filename, bool async = true);

}
