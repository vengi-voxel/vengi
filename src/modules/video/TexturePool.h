/**
 * @file
 */

#pragma once

#include "io/Filesystem.h"
#include "Texture.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace video {

/**
 * @ingroup Video
 */
class TexturePool {
private:
	io::FilesystemPtr _filesystem;
	std::unordered_map<std::string, TexturePtr> _cache;
public:
	TexturePool(const io::FilesystemPtr& filesystem);

	video::TexturePtr load(const std::string& name);

	bool init();
	void shutdown();
};

typedef std::shared_ptr<TexturePool> TexturePoolPtr;

}
