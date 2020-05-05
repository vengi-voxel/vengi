/**
 * @file
 */

#pragma once

#include "video/Cubemap.h"
#include "video/Buffer.h"
#include "SkyboxShader.h"

namespace video {
class Camera;
}

namespace render {

class Skybox {
private:
	video::Cubemap _cubemap;
	video::Buffer _vbo;
	shader::SkyboxShader _shader;
	int32_t _skyboxIndex = -1;
public:
	/**
	 * @brief Loads 6 textures that belongs to the sky.
	 * The naming schema must be "<filename>-cm-<side>" (where side is
	 * replaced with front, back, top, bottom, left and right)
	 */
	bool init(const char *filename);
	void shutdown();

	void bind(video::TextureUnit unit);
	void unbind(video::TextureUnit unit);
	video::Id cubemapHandle() const;

	void render(const video::Camera& camera);
};

inline video::Id Skybox::cubemapHandle() const {
	return _cubemap.handle();
}

}
