/**
 * @file
 */

#pragma once

#include "io/IOResource.h"
#include "io/File.h"
#include "core/App.h"
#include <memory>

namespace image {

class Image: public io::IOResource {
private:
	std::string _name;
	int _width;
	int _height;
	int _depth;
	uint8_t* _data;

public:
	Image(const std::string& name);
	~Image();

	void load(const io::FilePtr& file);
	void load(uint8_t* buffer, int length);

	static bool writePng(const char *name, const uint8_t *buffer, int width, int height, int depth);
	bool writePng() const;

	const std::string& name() const {
		return _name;
	}

	const uint8_t* data() const {
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
inline ImagePtr createImage(const std::string& name) {
	return std::make_shared<Image>(name);
}

inline ImagePtr loadImage(const io::FilePtr& file) {
	const ImagePtr& i = std::make_shared<Image>(file->getName());
	core::App::getInstance()->threadPool().enqueue([=] () { i->load(file); });
	return i;
}

inline ImagePtr loadImage(const std::string& filename) {
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open(filename);
	return loadImage(file);
}

}
