/**
 * @file
 */

#include "Texture.h"
#include "Compute.h"
#include "core/Assert.h"

namespace compute {

Texture::Texture(const TextureConfig& cfg, const glm::ivec2& size, const std::string& name) :
		_name(name), _size(size.x, size.y, 1) {
	_config = cfg;
	core_assert_msg(_config.type() == TextureType::Texture1D || _config.type() == TextureType::Texture2D, "Texture2D or Texture1D is needed as type");
}

Texture::Texture(const TextureConfig& cfg, const glm::ivec3& size, const std::string& name) :
		_name(name), _size(size) {
	_config = cfg;
	core_assert_msg(_size.z > 1, "Texture3D must have layers > 1, but %i is given", _size.z);
	core_assert_msg(_config.type() == TextureType::Texture3D, "Texture3D is needed as type");
}

Texture::~Texture() {
	core_assert_msg(_handle == InvalidId, "Texture %s was not properly shut down", _name.c_str());
	shutdown();
}

void Texture::shutdown() {
	compute::deleteTexture(_handle);
	compute::deleteSampler(_sampler);
}

bool Texture::upload(const uint8_t* data) {
	core_assert_msg(_handle == InvalidId, "Texture already created");
	_handle = compute::createTexture(*this, data);
	if (_handle == compute::InvalidId) {
		return false;
	}
	_sampler = compute::createSampler(_config);
	if (_sampler == compute::InvalidId) {
		return false;
	}
	return true;
}

}
