/**
 * @file
 */

#include "TexturePool.h"
#include "core/StringUtil.h"
#include "image/Image.h"

namespace video {

video::TexturePtr TexturePool::load(const core::String &name, bool emptyAsFallback) {
	auto i = _cache.find(name);
	if (i != _cache.end()) {
		return i->value;
	}

	const image::ImagePtr &image = loadImage(name);
	if (!image || image->isFailed()) {
		return TexturePtr();
	}
	TexturePtr texture = createTextureFromImage(image);
	if (!texture && emptyAsFallback) {
		texture = _empty;
	}
	_cache.put(name, texture);
	return texture;
}

image::ImagePtr TexturePool::loadImage(const core::String &name) {
	auto i = _images.find(name);
	if (i != _images.end()) {
		return i->value;
	}
	const image::ImagePtr &image = image::loadImage(name);
	_images.put(name, image);
	return image;
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
	_images.clear();
}

} // namespace video
