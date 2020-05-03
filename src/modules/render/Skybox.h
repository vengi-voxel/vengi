/**
 * @file
 */

#pragma once

#include "video/Cubemap.h"
#include "video/Buffer.h"
#include "video/Camera.h"
#include "SkyboxShader.h"

namespace video {
class Video;
}

namespace render {

class Skybox {
private:
	video::Cubemap _cubemap;
	video::Buffer _vbo;
	shader::SkyboxShader _shader;
	int32_t _skyboxIndex = -1;
public:
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
