/**
 * @file
 */

#include "TexturePool.h"

namespace video {

TexturePool::TexturePool() {
}

video::TexturePtr TexturePool::load(const char *name) {
	return video::TexturePtr();
}

bool TexturePool::init() {
	return true;
}

void TexturePool::shutdown() {
}

}
