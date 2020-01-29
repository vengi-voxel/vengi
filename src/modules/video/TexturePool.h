/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/io/Filesystem.h"
#include "Texture.h"
#include <memory>
#include "core/String.h"
#include <unordered_map>

namespace video {

/**
 * @ingroup Video
 */
class TexturePool : public core::IComponent {
private:
	io::FilesystemPtr _filesystem;
	std::unordered_map<std::string, TexturePtr> _cache;
public:
	TexturePool(const io::FilesystemPtr& filesystem);

	video::TexturePtr load(const core::String& name);

	bool init() override;
	void shutdown() override;
	void clear();
};

typedef std::shared_ptr<TexturePool> TexturePoolPtr;

}
