/**
 * @file
 */

#include "TexturePool.h"
#include "image/Image.h"

namespace video {

TexturePool::TexturePool(const io::FilesystemPtr& filesystem) :
		_filesystem(filesystem) {
}

video::TexturePtr TexturePool::load(const core::String& name, bool emptyAsFallback) {
	auto i = _cache.find(name);
	if (i != _cache.end()) {
		return i->value;
	}
	const io::FilePtr& file = _filesystem->open(name);
	const image::ImagePtr& image = image::loadImage(file, false);
	TexturePtr texture = createTextureFromImage(image);
	if (!texture && emptyAsFallback) {
		texture = _empty;
	}
	_cache.put(name, texture);
	return texture;
}

bool TexturePool::init() {
	_empty = createEmptyTexture("**empty**");
	return true;
}

void TexturePool::shutdown() {
	_empty = TexturePtr();
	clear();
}

void TexturePool::clear() {
	_cache.clear();
}

}
