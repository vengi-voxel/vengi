/**
 * @file
 */

#include "ClientEntity.h"
#include "animation/Vertex.h"
#include "core/App.h"
#include "core/io/Filesystem.h"

namespace frontend {

static inline std::string getCharacterLua(network::EntityType type) {
	const std::string& entityTypeStr = core::string::toLower(network::EnumNameEntityType(type));
	// TODO: get rid of the hardcoded prefix here
	const std::string& luaFilename = "human-male-" + entityTypeStr;
	const std::string& luaPath = animation::luaFilename(luaFilename.c_str());
	const std::string& lua = io::filesystem()->load(luaPath);
	if (lua.empty()) {
		// provide a fallback
		Log::warn("Could not load character settings from %s", luaPath.c_str());
		return getCharacterLua(network::EntityType::KNIGHT);
	}
	return lua;
}

ClientEntity::ClientEntity(const stock::StockDataProviderPtr& provider,
		const animation::CharacterCachePtr& characterCache, ClientEntityId id,
		network::EntityType type, const glm::vec3& pos, float orientation) :
		_id(id), _type(type), _position(pos), _orientation(orientation),
		_stock(provider), _characterCache(characterCache) {
	const std::string& lua = getCharacterLua(type);
	if (!_character.init(_characterCache, lua)) {
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
	_character.updateTool(_characterCache, _stock.inventory());
	_character.update(dt, _attrib);
}

uint32_t ClientEntity::bindVertexBuffers(const shader::CharacterShader& chrShader) {
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

}
