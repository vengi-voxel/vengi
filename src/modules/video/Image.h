#pragma once

#include "io/IOResource.h"
#include "io/File.h"
#include <memory>

namespace video {

class Image: public io::IOResource {
private:
	std::string _filename;
	int _width;
	int _height;
	int _depth;
	uint8_t* _data;
	bool _alpha = true;

	void load(const io::FilePtr& file);
public:
	Image(const std::string& filename);

	Image(uint8_t* data, int width, int height, int depth);

	~Image();

	void loadAsync();

	bool loadSync();

	const uint8_t* data() const {
		return _data;
	}

	inline int width() const {
		return _width;
	}

	inline int height() const {
		return _height;
	}

	inline bool hasAlpha() const {
		return _alpha;
	}
};

typedef std::shared_ptr<Image> ImagePtr;

}
