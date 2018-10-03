/**
 * @file
 */

#include "TexturePool.h"
#include "image/Image.h"

namespace video {

TexturePool::TexturePool(const io::FilesystemPtr& filesystem) :
		_filesystem(filesystem) {
}

video::TexturePtr TexturePool::load(const std::string& name) {
	auto i = _cache.find(name);
	if (i != _cache.end()) {
		return i->second;
	}
	const io::FilePtr& file = _filesystem->open(name);
	const image::ImagePtr& image = image::loadImage(file);
	const TexturePtr& texture = createTextureFromImage(image);
	_cache.insert(std::make_pair(name, texture));
	return texture;
}

bool TexturePool::init() {
	_cache.reserve(32);
	return true;
}

void TexturePool::shutdown() {
	clear();
}

void TexturePool::clear() {
	_cache.clear();
}

}
