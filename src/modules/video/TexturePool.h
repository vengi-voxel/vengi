/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/io/Filesystem.h"
#include "Texture.h"
#include "core/String.h"
#include "core/collection/StringMap.h"
#include <memory>

namespace video {

/**
 * @ingroup Video
 */
class TexturePool : public core::IComponent {
private:
	io::FilesystemPtr _filesystem;
	core::StringMap<TexturePtr> _cache;
public:
	TexturePool(const io::FilesystemPtr& filesystem);

	video::TexturePtr load(const core::String& name);

	bool init() override;
	void shutdown() override;
	void clear();
};

typedef std::shared_ptr<TexturePool> TexturePoolPtr;

}
