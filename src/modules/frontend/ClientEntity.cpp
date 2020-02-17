/**
 * @file
 */

#include "ClientEntity.h"
#include "animation/Vertex.h"
#include "core/App.h"
#include "core/io/Filesystem.h"
#include "animation/AnimationSettings.h"
#include "core/StringUtil.h"
#include "animation/AnimationCache.h"
#include "AnimationShaders.h"

namespace frontend {

static inline core::String getCharacterLua(network::EntityType type) {
	const core::String& entityTypeStr = network::EnumNameEntityType(type);
	core::String luaFilename = entityTypeStr.toLower();
	core::string::replaceAllChars(luaFilename, '_', '-');
	const core::String& luaPath = animation::luaFilename(luaFilename.c_str());
	const core::String& lua = io::filesystem()->load("chr/" + luaPath);
	if (lua.empty() && type != network::EntityType::HUMAN_MALE_KNIGHT) {
		// provide a fallback
		Log::warn("Could not load character settings from %s", luaPath.c_str());
		return getCharacterLua(network::EntityType::HUMAN_MALE_KNIGHT);
	}
	return lua;
}

ClientEntity::ClientEntity(const stock::StockDataProviderPtr& provider,
		const animation::AnimationCachePtr& animationCache, ClientEntityId id,
		network::EntityType type, const glm::vec3& pos, float orientation) :
		_id(id), _type(type), _position(pos), _orientation(orientation),
		_stock(provider), _animationCache(animationCache) {
	const core::String& lua = getCharacterLua(type);
	if (!_character.init(_animationCache, lua)) {
		Log::error("Failed to init the character");
	}
	if (!_stock.init()) {
		Log::error("Failed to init the stock");
	}
	_vertices = _vbo.create();
	_indices = _vbo.create(nullptr, 0, video::BufferType::IndexBuffer);
}

ClientEntity::~ClientEntity() {
	_character.shutdown();
	_stock.shutdown();
	_vbo.shutdown();
	_vertices = -1;
	_indices = -1;
}

void ClientEntity::update(uint64_t dt) {
	_attrib.update(dt);
	_character.updateTool(_animationCache, _stock);
	_character.update(dt, _attrib);
}

uint32_t ClientEntity::bindVertexBuffers(const shader::SkeletonShader& chrShader) {
	if (_vbo.attributes() == 0) {
		_vbo.addAttribute(chrShader.getPosAttribute(_vertices, &animation::Vertex::pos));
		video::Attribute color = chrShader.getColorIndexAttribute(_vertices, &animation::Vertex::colorIndex);
		color.typeIsInt = true;
		_vbo.addAttribute(color);
		video::Attribute boneId = chrShader.getBoneIdAttribute(_vertices, &animation::Vertex::boneId);
		boneId.typeIsInt = true;
		_vbo.addAttribute(boneId);
		video::Attribute ambientOcclusion = chrShader.getAmbientOcclusionAttribute(_vertices, &animation::Vertex::ambientOcclusion);
		ambientOcclusion.typeIsInt = true;
		_vbo.addAttribute(ambientOcclusion);
	}

	const animation::Indices& i = _character.indices();
	const animation::Vertices& v = _character.vertices();
	core_assert_always(_vbo.update(_indices, i));
	core_assert_always(_vbo.update(_vertices, v));

	_vbo.bind();
	return _vbo.elements(_indices, 1, sizeof(animation::IndexType));
}

void ClientEntity::unbindVertexBuffers() {
	_vbo.unbind();
}

void ClientEntity::userinfo(const core::String& key, const core::String& value) {
	_userinfo.put(key, value);
}

}
