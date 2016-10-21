/**
 * @file
 */

#pragma once

#include "io/IOResource.h"
#include "io/File.h"
#include "core/App.h"
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
	bool load(uint8_t* buffer, int length);

	static bool writePng(const char *name, const uint8_t *buffer, int width, int height, int depth);
	bool writePng() const;

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

inline ImagePtr loadImage(const io::FilePtr& file, bool async = true) {
	const ImagePtr& i = createEmptyImage(file->getName());
	if (async) {
		core::App::getInstance()->threadPool().enqueue([=] () { i->load(file); });
	} else {
		if (!i->load(file)) {
			Log::warn("Failed to load image %s", i->name().c_str());
		}
	}
	return i;
}

inline ImagePtr loadImage(const std::string& filename, bool async = true) {
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open(filename);
	return loadImage(file, async);
}

}
