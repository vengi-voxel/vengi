/**
 * @file
 */

#pragma once

#include "Renderer.h"

namespace video {

/**
 * @ingroup Video
 */
class Cubemap {
private:
	std::string _filename;
	Id _textureHandle = InvalidId;
public:
	~Cubemap();

	/**
	 * @brief Loads 6 textures that belongs to a cubemap.
	 * The naming schema must be "<filename>-cm-<side>" (where side is
	 * replaced with front, back, top, bottom, left and right)
	 */
	bool init(const char *filename);
	void shutdown();

	video::Id handle() const;

	void bind(video::TextureUnit texUnit = video::TextureUnit::Zero);
	void unbind(video::TextureUnit texUnit = video::TextureUnit::Zero);
};

inline video::Id Cubemap::handle() const {
	return _textureHandle;
}

}
