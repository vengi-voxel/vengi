/**
 * @file
 */

#include "Character.h"

#include <unordered_set>

#include "animation/Animation.h"

#include "core/Common.h"
#include "core/GLM.h"

#include "stock/Item.h"

#include "anim/Idle.h"
#include "anim/Jump.h"
#include "anim/Run.h"
#include "anim/Glide.h"
#include "anim/Tool.h"

namespace animation {

bool Character::init(const AnimationCachePtr& cache, const std::string& luaString) {
	if (!initSettings(luaString)) {
		return false;
	}
	return initMesh(cache);
}

bool Character::initSettings(const std::string& luaString) {
	AnimationSettings settings;
	CharacterSkeletonAttribute attributes;
	if (loadCharacterSettings(luaString, settings, attributes)) {
		_settings = settings;
		_attributes = attributes;
		return true;
	}
	Log::warn("Failed to load the character settings");
	return false;
}

bool Character::loadGlider(const AnimationCachePtr& cache, const AnimationSettings& settings, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]) {
	const int idx = settings.getIdxForName("glider");
	if (idx < 0 || idx >= (int)AnimationSettings::MAX_ENTRIES) {
		return false;
	}
	const char *fullPath = "models/glider.vox";
	voxel::Mesh& mesh = cache->cacheEntry(fullPath);
	if (mesh.getNoOfVertices() > 0) {
		meshes[idx] = &mesh;
		return true;
	}
	if (cache->loadMesh(fullPath, mesh)) {
		meshes[idx] = &mesh;
		return true;
	}
	meshes[idx] = nullptr;
	Log::error("Failed to load glider");
	return false;
}

bool Character::initMesh(const AnimationCachePtr& cache) {
	if (!cache->getBoneModel(_settings, _vertices, _indices, [&] (const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]) {
		return loadGlider(cache, _settings, meshes);
	})) {
		Log::warn("Failed to load the character model");
		return false;
	}
	_toolVerticesOffset = _vertices.size();
	_toolIndicesOffset = _indices.size();
	return true;
}

bool Character::updateTool(const AnimationCachePtr& cache, const stock::Inventory& inv) {
	// TODO: id resolving via constants
	// weapon/right hand  see stock.lua
	const stock::Container* container = inv.container(2);
	if (container == nullptr) {
		return false;
	}
	const stock::Container::ContainerItems& items = container->items();
	if (items.size() != 1) {
		return false;
	}

	const auto& citem = items[0];
	const stock::ItemId id = citem.item->id();
	if (id == _toolId) {
		return true;
	}

	_toolId = id;
	_toolAnim = toToolAnimationEnum(citem.item->label("anim"));
	if (_toolAnim == ToolAnimationType::Max) {
		Log::warn("Invalid label 'anim' found on item '%s'", citem.item->name());
		_toolAnim = ToolAnimationType::None;
	}

	const char *itemName = citem.item->name();
	char fullPath[128];
	if (!core::string::formatBuf(fullPath, sizeof(fullPath), "models/items/%s.vox", itemName)) {
		Log::error("Failed to initialize the item path buffer. Can't load item %s.", itemName);
		return false;
	}
	if (!cache->getModel(fullPath, BoneId::Tool, _toolVertices, _toolIndices)) {
		Log::warn("Could not get item model for %s", citem.item->name());
		return false;
	}

	_vertices.resize(_toolVerticesOffset + _toolVertices.size());
	for (size_t i = 0; i < _toolVertices.size(); ++i) {
		_vertices[_toolVerticesOffset + i] = _toolVertices[i];
	}

	_indices.resize(_toolIndicesOffset + _toolIndices.size());
	for (size_t i = 0; i < _toolIndices.size(); ++i) {
		_indices[_toolIndicesOffset + i] = _toolIndices[i] + _toolVerticesOffset;
	}

	Log::debug("Added %i vertices for the active tool", (int)_toolVertices.size());
	return true;
}

void Character::update(uint64_t dt, const attrib::ShadowAttributes& attrib) {
	static float globalTimeSeconds = 0.0f;
	const float animTimeSeconds = float(dt) / 1000.0f;

	const CharacterSkeleton old = _skeleton;

	const float velocity = (float)attrib.current(attrib::Type::SPEED);

	switch (_anim) {
	case Animation::Idle:
		chr::idle::update(globalTimeSeconds, _skeleton, _attributes);
		break;
	case Animation::Jump:
		chr::jump::update(globalTimeSeconds, _skeleton, _attributes);
		break;
	case Animation::Run:
		chr::run::update(globalTimeSeconds, velocity, _skeleton, _attributes);
		break;
	case Animation::Glide:
		chr::glide::update(globalTimeSeconds, _skeleton, _attributes);
		break;
	case Animation::Tool:
		if (_toolAnim == ToolAnimationType::None || _toolAnim == ToolAnimationType::Max) {
			chr::idle::update(globalTimeSeconds, _skeleton, _attributes);
		} else {
			chr::tool::update(globalTimeSeconds, _toolAnim, _skeleton, _attributes);
		}
		break;
	default:
		break;
	}

	if (globalTimeSeconds > 0.0f) {
		const float ticksPerSecond = 15.0f;
		_skeleton.lerp(old, glm::min(1.0f, animTimeSeconds * ticksPerSecond));
	}

	globalTimeSeconds += animTimeSeconds;
}

void Character::shutdown() {
	_toolId = (stock::ItemId)-1;
	_toolAnim = ToolAnimationType::Max;
	_toolVerticesOffset = 0u;
	_toolIndicesOffset = 0u;
}

const Skeleton& Character::skeleton() const {
	return _skeleton;
}

}
