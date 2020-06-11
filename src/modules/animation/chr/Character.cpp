/**
 * @file
 */

#include "Character.h"
#include "animation/Animation.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "stock/Item.h"
#include "anim/Idle.h"
#include "anim/Jump.h"
#include "anim/Run.h"
#include "anim/Glide.h"
#include "anim/Swim.h"
#include "anim/Tool.h"

namespace animation {

bool Character::initSettings(const core::String& luaString) {
	AnimationSettings settings;
	CharacterSkeletonAttribute attributes;
	if (loadAnimationSettings(luaString, settings, &attributes)) {
		if (!attributes.init()) {
			return false;
		}
		_settings = settings;
		_attributes = attributes;
		return true;
	}
	Log::warn("Failed to load the character settings");
	return false;
}

bool Character::loadGlider(const AnimationCachePtr& cache, const AnimationSettings& settings, const voxel::Mesh* (&meshes)[AnimationSettings::MAX_ENTRIES]) {
	const int idx = settings.getMeshTypeIdxForName("glider");
	if (idx < 0 || idx >= (int)AnimationSettings::MAX_ENTRIES) {
		return false;
	}
	// TODO: model via inventory
	const char *fullPath = "models/glider.vox";
	meshes[idx] = cache->getMesh(fullPath);
	if (meshes[idx] == nullptr) {
		Log::error("Failed to load glider");
		return false;
	}
	return true;
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

	// ensure the bones are in a sane state - needed for getting the aabb right
	chr::idle::update(_globalTimeSeconds, _skeleton, _attributes);

	return true;
}

bool Character::updateTool(const AnimationCachePtr& cache, const stock::Stock& stock) {
	const int containerId = stock.containerId("tool");
	if (containerId < 0) {
		return false;
	}
	const stock::Container* container = stock.inventory().container(containerId);
	if (container == nullptr) {
		return false;
	}
	const stock::Container::ContainerItems& items = container->items();
	if (items.size() < 1) {
		return false;
	}

	// TODO: pick the biggest
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
	if (!core::string::formatBuf(fullPath, sizeof(fullPath), "models/items/%s", itemName)) {
		Log::error("Failed to initialize the item path buffer. Can't load item %s.", itemName);
		return false;
	}
	if (!cache->getModel(_settings, fullPath, BoneId::Tool, _toolVertices, _toolIndices)) {
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

void Character::update(double deltaSeconds, const attrib::ShadowAttributes& attrib) {
	const CharacterSkeleton old = _skeleton;

	const double velocity = attrib.current(attrib::Type::SPEED);

	// TODO: lerp the animations
	for (int i = 0; i <= core::enumVal(Animation::MAX); ++i) {
		const double aTime = _animationTimes[i];
		if (aTime < _globalTimeSeconds) {
			continue;
		}
		const Animation anim = (Animation)i;
		switch (anim) {
		case Animation::IDLE:
			chr::idle::update(_globalTimeSeconds, _skeleton, _attributes);
			break;
		case Animation::JUMP:
			chr::jump::update(_globalTimeSeconds, _skeleton, _attributes);
			break;
		case Animation::RUN:
			chr::run::update(_globalTimeSeconds, velocity, _skeleton, _attributes);
			break;
		case Animation::SWIM:
			chr::swim::update(_globalTimeSeconds, velocity, _skeleton, _attributes);
			break;
		case Animation::GLIDE:
			chr::glide::update(_globalTimeSeconds, _skeleton, _attributes);
			break;
		case Animation::TOOL:
			if (_toolAnim == ToolAnimationType::None || _toolAnim == ToolAnimationType::Max) {
				chr::idle::update(_globalTimeSeconds, _skeleton, _attributes);
			} else {
				chr::tool::update(_globalTimeSeconds, _toolAnim, _skeleton, _attributes);
			}
			break;
		default:
			break;
		}
	}

	if (_globalTimeSeconds > 0.0) {
		_skeleton.lerp(old, deltaSeconds);
	}

	_globalTimeSeconds += deltaSeconds;
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
