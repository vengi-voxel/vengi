/**
 * @file
 */

#include "TexturePool.h"
#include "image/Image.h"

namespace video {

TexturePool::TexturePool(const io::FilesystemPtr& filesystem) :
		_filesystem(filesystem) {
}

video::TexturePtr TexturePool::load(const core::String& name) {
	auto i = _cache.find(name);
	if (i != _cache.end()) {
		return i->value;
	}
	const io::FilePtr& file = _filesystem->open(name);
	const image::ImagePtr& image = image::loadImage(file, false);
	const TexturePtr& texture = createTextureFromImage(image);
	_cache.put(name, texture);
	return texture;
}

bool TexturePool::init() {
	return true;
}

void TexturePool::shutdown() {
	clear();
}

void TexturePool::clear() {
	_cache.clear();
}

}
