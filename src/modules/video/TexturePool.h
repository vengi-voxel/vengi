/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "io/Filesystem.h"
#include "Texture.h"
#include "image/Image.h"
#include "core/String.h"
#include "core/SharedPtr.h"
#include "core/collection/StringMap.h"

namespace video {

/**
 * @ingroup Video
 */
class TexturePool : public core::IComponent {
private:
	core::StringMap<TexturePtr> _cache;
	core::StringMap<image::ImagePtr> _images;
	TexturePtr _empty;
public:
	video::TexturePtr load(const core::String& name, bool emptyAsFallback = true);
	image::ImagePtr loadImage(const core::String& name);

	const core::StringMap<TexturePtr> &cache() {
		return _cache;
	}

	bool init() override;
	void shutdown() override;
	void clear();
};

typedef core::SharedPtr<TexturePool> TexturePoolPtr;

}
