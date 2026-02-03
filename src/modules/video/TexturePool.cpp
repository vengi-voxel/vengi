/**
 * @file
 */

#include "TexturePool.h"
#include "command/Command.h"
#include "core/Log.h"
#include "image/Image.h"
#include "io/MemoryReadStream.h"
#include "app/I18N.h"

namespace video {

bool TexturePool::has(const core::String& name) const {
	return _cache.hasKey(name);
}

video::TexturePtr TexturePool::get(const core::String &name) {
	auto i = _cache.find(name);
	if (i != _cache.end()) {
		return i->value;
	}
	return _empty;
}

video::TexturePtr TexturePool::load(const core::String& name, const uint8_t *rgba, size_t size) {
	auto i = _cache.find(name);
	if (i != _cache.end()) {
		return i->value;
	}
	const image::ImagePtr &image = loadImage(name, rgba, size);
	if (!image || image->isFailed()) {
		return TexturePtr();
	}
	TexturePtr texture = createTextureFromImage(image);
	_cache.put(name, texture);
	return texture;
}

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

video::TexturePtr TexturePool::addImage(const image::ImagePtr &image) {
	_images.put(image->name(), image);
	return load(image->name());
}

image::ImagePtr TexturePool::loadImage(const core::String& name, const uint8_t *rgba, size_t size) {
	auto i = _images.find(name);
	if (i != _images.end()) {
		return i->value;
	}
	io::MemoryReadStream stream(rgba, size);
	const image::ImagePtr &image = image::loadImage(name, stream);
	_images.put(name, image);
	return image;
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

void TexturePool::construct() {
	command::Command::registerCommand("texturepoollist")
		.setHandler([this](const command::CommandArgs &args) {
			Log::info("TexturePool");
			for (const auto &e : _cache) {
				Log::info("- %s\n", e->first.c_str());
			}
		}).setHelp(_("Show all images in the texture pool"));
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
