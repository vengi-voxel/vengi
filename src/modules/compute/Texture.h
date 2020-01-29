/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/GLM.h"
#include "TextureConfig.h"
#include <memory>

namespace compute {

class Texture {
private:
	core::String _name;
	Id _handle = InvalidId;
	Id _sampler = InvalidId;
	glm::ivec3 _size;
	TextureConfig _config;
public:
	Texture(const TextureConfig& cfg, const glm::ivec2& size, const core::String& name = "");
	Texture(const TextureConfig& cfg, const glm::ivec3& size, const core::String& name = "");
	~Texture();
	void shutdown();

	operator Id () const;
	TextureDataFormat dataformat() const;
	TextureFormat format() const;
	int width() const;
	int height() const;
	int layers() const;
	TextureType type() const;
	Id handle() const;
	Id sampler() const;

	bool upload(const uint8_t* data);
};

inline int Texture::layers() const {
	return _size.z;
}

inline TextureType Texture::type() const {
	return _config.type();
}

inline Texture::operator Id() const {
	return _handle;
}

inline TextureDataFormat Texture::dataformat() const {
	return _config.dataformat();
}

inline TextureFormat Texture::format() const {
	return _config.format();
}

inline int Texture::width() const {
	return _size.x;
}

inline int Texture::height() const {
	return _size.y;
}

inline Id Texture::handle() const {
	return _handle;
}

inline Id Texture::sampler() const {
	return _sampler;
}

typedef std::shared_ptr<Texture> TexturePtr;

}
