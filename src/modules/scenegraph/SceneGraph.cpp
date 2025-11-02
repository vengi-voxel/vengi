/**
 * @file
 */

#include "SceneGraph.h"
#include "SceneUtil.h"
#include "app/Async.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/Physics.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeVisitor.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/ext/quaternion_relational.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_ASSERT core_assert
#include "external/stb_rect_pack.h"

namespace scenegraph {

SceneGraph::MergeResult::MergeResult(voxel::RawVolume *volume, const palette::Palette &_palette,
									 const palette::NormalPalette &_normalPalette)
	: _volume(volume), palette(_palette), normalPalette(_normalPalette) {
}

SceneGraph::MergeResult::~MergeResult() {
	delete _volume;
}

voxel::RawVolume *SceneGraph::MergeResult::volume() const {
	voxel::RawVolume *tmp = _volume;
	_volume = nullptr;
	return tmp;
}

SceneGraph::SceneGraph() : _activeAnimation(DEFAULT_ANIMATION) {
	clear();
}

SceneGraph::~SceneGraph() {
	for (const auto &entry : _nodes) {
		entry->value.release();
	}
	_nodes.clear();
	_listeners.clear();
}

SceneGraph::SceneGraph(SceneGraph &&other) noexcept
	: _nodes(core::move(other._nodes)), _nextNodeId(other._nextNodeId), _activeNodeId(other._activeNodeId),
	  _animations(core::move(other._animations)), _activeAnimation(core::move(other._activeAnimation)),
	  _cachedMaxFrame(other._cachedMaxFrame) {
	other._nextNodeId = 0;
	other._activeNodeId = InvalidNodeId;
	_dirty = other.dirty();
}

SceneGraph &SceneGraph::operator=(SceneGraph &&other) noexcept {
	if (this != &other) {
		_nodes = core::move(other._nodes);
		_nextNodeId = other._nextNodeId;
		other._nextNodeId = 0;
		_activeNodeId = other._activeNodeId;
		other._activeNodeId = InvalidNodeId;
		_animations = core::move(other._animations);
		_activeAnimation = core::move(other._activeAnimation);
		_cachedMaxFrame = other._cachedMaxFrame;
		_dirty = other.dirty();
	}
	return *this;
}

void SceneGraph::registerListener(SceneGraphListener *listener) {
	for (SceneGraphListener *l : _listeners) {
		if (l == listener) {
			Log::error("Listener is already registered");
			return;
		}
	}
	_listeners.push_back(listener);
}

bool SceneGraph::isRegistered(SceneGraphListener *listener) const {
	return core::find(_listeners.begin(), _listeners.end(), listener) != _listeners.end();
}

void SceneGraph::unregisterListener(SceneGraphListener *listener) {
	for (auto iter = _listeners.begin(); iter != _listeners.end(); ++iter) {
		if (*iter == listener) {
			_listeners.erase(iter);
			return;
		}
	}
	Log::error("Listener not found - could not unregister");
}

bool SceneGraph::setAnimation(const core::String &animation) {
	if (animation.empty()) {
		Log::debug("Can't set empty animation");
		return false;
	}
	if (!hasAnimation(animation)) {
		Log::debug("Animation %s not found", animation.c_str());
		return false;
	}
	_activeAnimation = animation;
	for (const auto &entry : _nodes) {
		entry->value.setAnimation(animation);
	}
	markMaxFramesDirty();
	return true;
}

const SceneGraphAnimationIds &SceneGraph::animations() const {
	return _animations;
}

bool SceneGraph::duplicateAnimation(const core::String &animation, const core::String &newName) {
	if (animation.empty() || newName.empty()) {
		Log::error("Invalid animation names given");
		return false;
	}
	if (!hasAnimation(animation)) {
		Log::error("Animation %s not found", animation.c_str());
		return false;
	}
	if (hasAnimation(newName)) {
		Log::error("Animation %s already exists", newName.c_str());
		return false;
	}
	Log::debug("Add new animation %s by duplicating from %s", newName.c_str(), animation.c_str());
	_animations.push_back(newName);
	for (const auto &entry : _nodes) {
		SceneGraphNode &node = entry->value;
		if (!node.duplicateKeyFrames(animation, newName)) {
			Log::warn("Failed to set keyframes for node %i and animation %s", node.id(), animation.c_str());
		}
	}
	for (SceneGraphListener *listener : _listeners) {
		listener->onAnimationAdded(newName);
	}
	updateTransforms_r(node(0));
	return true;
}

bool SceneGraph::addAnimation(const core::String &animation) {
	if (animation.empty()) {
		return false;
	}
	if (hasAnimation(animation)) {
		return false;
	}
	_animations.push_back(animation);
	for (SceneGraphListener *listener : _listeners) {
		listener->onAnimationAdded(animation);
	}
	return true;
}

bool SceneGraph::setAnimations(const core::DynamicArray<core::String> &animations) {
	_animations = animations;
	if (_animations.empty()) {
		addAnimation(DEFAULT_ANIMATION);
		setAnimation(DEFAULT_ANIMATION);
	} else {
		if (!hasAnimation(_activeAnimation)) {
			setAnimation(*_animations.begin());
		}
	}
	return true;
}

bool SceneGraph::hasAnimation(const core::String &animation) const {
	return core::find(_animations.begin(), _animations.end(), animation) != _animations.end();
}

bool SceneGraph::removeAnimation(const core::String &animation) {
	auto iter = core::find(_animations.begin(), _animations.end(), animation);
	if (iter == _animations.end()) {
		return false;
	}
	_animations.erase(iter);
	for (const auto &entry : _nodes) {
		entry->value.removeAnimation(animation);
	}
	if (_animations.empty()) {
		addAnimation(DEFAULT_ANIMATION);
		setAnimation(DEFAULT_ANIMATION);
	} else if (_activeAnimation == animation) {
		setAnimation(*_animations.begin());
	}
	for (SceneGraphListener *listener : _listeners) {
		listener->onAnimationRemoved(animation);
	}
	return true;
}

bool SceneGraph::hasAnimations() const {
	for (const core::String &animation : animations()) {
		for (const auto &entry : _nodes) {
			if (entry->value.keyFrames(animation).size() > 1) {
				return true;
			}
		}
	}
	return false;
}

const core::String &SceneGraph::activeAnimation() const {
	return _activeAnimation;
}

void SceneGraph::markMaxFramesDirty() {
	_cachedMaxFrame = -1;
}

FrameIndex SceneGraph::maxFrames() const {
	if (_cachedMaxFrame <= -1) {
		for (const auto &entry : nodes()) {
			const SceneGraphNode &node = entry->second;
			if (node.allKeyFrames().empty()) {
				continue;
			}
			const FrameIndex maxFrame = node.maxFrame();
			_cachedMaxFrame = core_max(maxFrame, _cachedMaxFrame);
		}
	}
	return _cachedMaxFrame;
}

int SceneGraph::activeNode() const {
	return _activeNodeId;
}

bool SceneGraph::setActiveNode(int nodeId) {
	if (!hasNode(nodeId)) {
		return false;
	}
	_activeNodeId = nodeId;
	return true;
}

SceneGraphNode *SceneGraph::firstModelNode() const {
	auto iter = begin(SceneGraphNodeType::Model);
	if (iter != end()) {
		return &(*iter);
	}
	return nullptr;
}

palette::Palette &SceneGraph::firstPalette() const {
	SceneGraphNode *node = firstModelNode();
	if (node == nullptr) {
		return voxel::getPalette();
	}
	return node->palette();
}

const core::UUID &SceneGraph::uuid(int nodeId) const {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		return _emptyUUID;
	}
	return iter->value.uuid();
}

SceneGraphNode &SceneGraph::node(int nodeId) const {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		Log::error("No node for id %i found in the scene graph - returning root node", nodeId);
		return _nodes.find(0)->value;
	}
	return iter->value;
}

bool SceneGraph::hasNode(int nodeId) const {
	if (nodeId == InvalidNodeId) {
		return false;
	}
	return _nodes.hasKey(nodeId);
}

const SceneGraphNode &SceneGraph::root() const {
	return node(0);
}

int SceneGraph::prevModelNode(int nodeId) const {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		return InvalidNodeId;
	}
	const SceneGraphNode &ownNode = iter->second;
	if (ownNode.parent() == InvalidNodeId) {
		return InvalidNodeId;
	}
	int lastChild = InvalidNodeId;
	const SceneGraphNode &parentNode = node(ownNode.parent());
	const auto &children = parentNode.children();
	for (int child : children) {
		if (child == nodeId) {
			if (lastChild == InvalidNodeId) {
				break;
			}
			return lastChild;
		}
		if (node(child).isAnyModelNode()) {
			lastChild = child;
			continue;
		}
	}
	if (parentNode.isAnyModelNode()) {
		return parentNode.id();
	}
	return InvalidNodeId;
}

int SceneGraph::nextModelNode(int nodeId) const {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		return InvalidNodeId;
	}
	const SceneGraphNode &ownNode = iter->second;
	if (ownNode.parent() == InvalidNodeId) {
		return InvalidNodeId;
	}
	bool foundOwnChild = false;
	const auto &children = node(ownNode.parent()).children();
	for (int child : children) {
		if (child == nodeId) {
			foundOwnChild = true;
			continue;
		}
		if (foundOwnChild) {
			if (node(child).isAnyModelNode()) {
				return child;
			}
		}
	}
	bool found = false;
	for (iterator modelIter = beginModel(); modelIter != end(); ++modelIter) {
		if ((*modelIter).id() == nodeId) {
			found = true;
			continue;
		}
		if (found) {
			return (*modelIter).id();
		}
	}
	return InvalidNodeId;
}

scenegraph::SceneGraphNodeCamera *SceneGraph::activeCameraNode() const {
	int nodeId = activeNode();
	if (nodeId == InvalidNodeId) {
		return nullptr;
	}
	scenegraph::SceneGraphNode &n = node(nodeId);
	if (n.isCameraNode()) {
		return &toCameraNode(n);
	}
	return nullptr;
}

math::AABB<float> SceneGraph::calculateGroupAABB(const SceneGraphNode &node, FrameIndex frameIdx) const {
	const FrameTransform &transform = transformForFrame(node, frameIdx);
	math::AABB<float> aabb;
	if (node.isAnyModelNode()) {
		const voxel::Region &nregion = resolveRegion(node);
		const math::OBBF &obb = toOBB(true, nregion, node.pivot(), transform);
		aabb = toAABB(obb);
	} else {
		aabb = math::AABB<float>(transform.translation(), transform.translation() + 1.0f);
	}

	for (int child : node.children()) {
		const SceneGraphNode &cnode = this->node(child);
		const math::AABB<float> &caabb = calculateGroupAABB(cnode, frameIdx);
		if (caabb.isValid()) {
			if (aabb.isValid()) {
				aabb.accumulate(caabb);
			} else {
				aabb = caabb;
			}
		}
	}

	return aabb;
}

// TODO: PERF: sweeping
void SceneGraph::getCollisionNodes(CollisionNodes &out, FrameIndex frameIdx) const {
	if (frameIdx == InvalidFrame) {
		out.reserve(nodes().size());
		for (const auto &e : nodes()) {
			const scenegraph::SceneGraphNode &node = e->second;
			if (!node.visible() || !node.isAnyModelNode()) {
				continue;
			}
			const voxel::RawVolume *volume = resolveVolume(node);
			if (!volume) {
				continue;
			}
			out.emplace_back(volume, glm::mat4(1.0f));
		}
		return;
	}

	core::DynamicArray<const scenegraph::SceneGraphNode *> cnodes;
	for (const auto &e : nodes()) {
		const scenegraph::SceneGraphNode &node = e->second;
		if (!node.visible() || !node.isAnyModelNode()) {
			continue;
		}
		const voxel::RawVolume *volume = resolveVolume(node);
		if (!volume) {
			continue;
		}
		cnodes.push_back(&node);
	}

	out.resize(cnodes.size());
	app::for_parallel(0, (int)cnodes.size(), [&](int begin, int end) {
		for (int i = begin; i < end; ++i) {
			const scenegraph::SceneGraphNode &node = *cnodes[i];
			const FrameTransform &transform = transformForFrame(node, frameIdx);
			const voxel::RawVolume *volume = resolveVolume(node);
			const glm::ivec3 &dimensions = volume->region().getDimensionsInVoxels();
			const glm::mat4 &worldMat = transform.calculateWorldMatrix(node.pivot(), dimensions);
			const glm::mat4 &inverse = glm::inverse(worldMat);
			out[i] = CollisionNode(volume, inverse);
		}
	});
}

FrameTransform SceneGraph::transformForFrame(const SceneGraphNode &node, FrameIndex frameIdx) const {
	// TODO: SCENEGRAPH: ik solver https://github.com/vengi-voxel/vengi/issues/182
	// and https://github.com/vengi-voxel/vengi/issues/265
	// TODO: SCENEGRAPH: solve flipping of child transforms if parent has rotation applied - see
	// https://github.com/vengi-voxel/vengi/issues/420
	FrameTransform parentTransform;
	if (node.parent() == InvalidNodeId) {
		parentTransform.setWorldMatrix(glm::mat4(1.0f));
	} else {
		parentTransform = transformForFrame(this->node(node.parent()), frameIdx);
	}

	FrameTransform transform;
	KeyFrameIndex keyFrameIdx = InvalidKeyFrame;
	if (node.keyFrames().size() == 1) {
		const SceneGraphKeyFrame *kf = node.keyFrame(0);
		transform.setWorldMatrix(parentTransform.worldMatrix() * kf->transform().localMatrix());
	} else if (node.hasKeyFrameForFrame(frameIdx, &keyFrameIdx)) {
		const SceneGraphKeyFrame *kf = node.keyFrame(keyFrameIdx);
		transform.setWorldMatrix(parentTransform.worldMatrix() * kf->transform().localMatrix());
	} else {
		const KeyFrameIndex start = node.previousKeyFrameForFrame(frameIdx);
		const KeyFrameIndex end = node.nextKeyFrameForFrame(frameIdx);
		if (start == end) {
			const SceneGraphKeyFrame *kf = node.keyFrame(start);
			transform.setWorldMatrix(parentTransform.worldMatrix() * kf->transform().localMatrix());
		} else {
			const SceneGraphKeyFrame *source = node.keyFrame(start);
			const SceneGraphKeyFrame *target = node.keyFrame(end);
			core_assert_always(source && target);
			const InterpolationType interpolationType = source->interpolation;

			glm::quat tgtLocalOrientation = target->transform().localOrientation();
			const glm::quat &srcLocalOrientation = source->transform().localOrientation();
			if (source->longRotation) {
				if (glm::dot(tgtLocalOrientation, srcLocalOrientation) < 0.0f) {
					tgtLocalOrientation *= -1.0f;
				}
			}

			const double deltaFrame = scenegraph::interpolate(interpolationType, (double)frameIdx, (double)source->frameIdx, (double)target->frameIdx);
			const float lerpFactor = glm::clamp((float)(deltaFrame - (double)source->frameIdx), 0.0f, 1.0f);

			const glm::vec3 translation = glm::mix(source->transform().localTranslation(), target->transform().localTranslation(), lerpFactor);
			const glm::quat orientation = glm::slerp(srcLocalOrientation, tgtLocalOrientation, lerpFactor);
			const glm::vec3 scale = glm::mix(source->transform().localScale(), target->transform().localScale(), lerpFactor);
			transform.setWorldMatrix(parentTransform.worldMatrix() * (glm::translate(translation) * glm::mat4_cast(orientation) * glm::scale(scale)));
		}
	}
	return transform;
}

void SceneGraph::updateTransforms_r(SceneGraphNode &n) {
	for (SceneGraphKeyFrame &keyframe : *n.keyFrames()) {
		keyframe.transform().update(*this, n, keyframe.frameIdx, true);
	}
	for (int childrenId : n.children()) {
		updateTransforms_r(node(childrenId));
	}
}

void SceneGraph::updateTransforms() {
	const core::String animId = _activeAnimation;
	for (const core::String &animation : animations()) {
		core_assert_always(setAnimation(animation));
		updateTransforms_r(node(0));
	}
	core_assert_always(setAnimation(animId));
}

voxel::Region SceneGraph::maxRegion() const {
	int maxVoxels = 0;
	voxel::Region r;
	for (const auto &n : nodes()) {
		const SceneGraphNode &node = n->second;
		if (!node.isModelNode()) {
			continue;
		}
		if (node.region().voxels() > maxVoxels) {
			maxVoxels = node.region().voxels();
			r = node.region();
		}
	}
	return r;
}

voxel::Region SceneGraph::calcRegion() const {
	voxel::Region r;
	bool validVolume = false;
	for (const auto &n : nodes()) {
		const SceneGraphNode &node = n->second;
		if (!node.isModelNode()) {
			continue;
		}
		if (validVolume) {
			r.accumulate(node.region());
			continue;
		}
		r = node.region();
		validVolume = true;
	}
	return r;
}

const voxel::Region &SceneGraph::region() const {
	if (_regionDirty) {
		_region = calcRegion();
		_regionDirty = false;
	}
	return _region;
}

voxel::Region SceneGraph::sceneRegion(KeyFrameIndex keyFrameIdx, bool onlyVisible) const {
	voxel::Region r;
	bool validVolume = false;
	for (const auto &n : nodes()) {
		const SceneGraphNode &node = n->second;
		if (!node.isAnyModelNode()) {
			continue;
		}
		if (onlyVisible && !node.visible()) {
			continue;
		}
		const voxel::Region &nodeRegion = sceneRegion(node, keyFrameIdx);
		if (validVolume) {
			r.accumulate(nodeRegion);
			continue;
		}
		r = nodeRegion;
		validVolume = true;
	}
	return r;
}

math::OBBF SceneGraph::sceneOBB(const SceneGraphNode &node, FrameIndex frameIdx) const {
	const auto &transform = transformForFrame(node, frameIdx);
	const voxel::Region &region = resolveRegion(node);
	const math::OBBF &obb = toOBB(true, region, node.pivot(), transform);
	return obb;
}

voxel::Region SceneGraph::sceneRegion(const SceneGraphNode &node, FrameIndex frameIdx) const {
	return toRegion(sceneOBB(node, frameIdx));
}

void SceneGraph::fixErrors() {
	Log::warn("Attempt to fix errors in the scene graph");
	_nodes.for_parallel([](const SceneGraphNodes::key_type &key, SceneGraphNode &value) {
		value.fixErrors();
	});
	updateTransforms();
}

bool SceneGraph::validate() const {
	bool valid = true;
	_nodes.for_parallel([&] (const SceneGraphNodes::key_type &key, const SceneGraphNode &value) {
		if (!value.validate()) {
			valid = false;
		}
	});
	return true;
}

SceneGraphNode *SceneGraph::findNodeByPropertyValue(const core::String &key, const core::String &value) const {
	for (const auto &entry : _nodes) {
		if (entry->value.property(key) == value) {
			return &entry->value;
		}
	}
	return nullptr;
}

SceneGraphNode *SceneGraph::findNodeByName(const core::String &name) {
	for (const auto &entry : _nodes) {
		Log::trace("node name: %s", entry->value.name().c_str());
		if (entry->value.name() == name) {
			return &entry->value;
		}
	}
	return nullptr;
}

SceneGraphNode *SceneGraph::findNodeByUUID(const core::UUID &uuid) {
	for (const auto &entry : _nodes) {
		const core::String &uuidStr = entry->value.uuid().str();
		Log::trace("node uuid: %s", uuidStr.c_str());
		if (entry->value.uuid() == uuid) {
			return &entry->value;
		}
	}
	return nullptr;
}

const SceneGraphNode *SceneGraph::findNodeByName(const core::String &name) const {
	for (const auto &entry : _nodes) {
		Log::trace("node name: %s", entry->value.name().c_str());
		if (entry->value.name() == name) {
			return &entry->value;
		}
	}
	return nullptr;
}

SceneGraphNode *SceneGraph::first() {
	if (!_nodes.empty()) {
		return &_nodes.begin()->value;
	}
	return nullptr;
}

void SceneGraph::setRootUUID(const core::UUID &uuid) {
	if (!uuid.isValid()) {
		return;
	}
	auto iter = _nodes.find(0);
	if (iter == _nodes.end()) {
		return;
	}
	iter->value._uuid = uuid;
}

int SceneGraph::emplace(SceneGraphNode &&node, int parent) {
	const SceneGraphNodeType type = node.type();
	core_assert_msg((int)type < (int)SceneGraphNodeType::Max, "%i", (int)type);
	if (type == SceneGraphNodeType::Root && _nextNodeId != 0) {
		Log::error("No second root node is allowed in the scene graph");
		node.release();
		return InvalidNodeId;
	}
	if (type == SceneGraphNodeType::Model) {
		core_assert(node.volume() != nullptr);
		core_assert(node.region().isValid());
		if (node.volume() == nullptr) {
			return InvalidNodeId;
		}
	}
	const int nodeId = _nextNodeId;
	if (parent >= nodeId) {
		Log::error("Invalid parent id given: %i", parent);
		node.release();
		return InvalidNodeId;
	}

	if (findNodeByUUID(node.uuid()) != nullptr) {
		const core::String &uuidStr = node.uuid().str();
		Log::error("Node with UUID %s already exists in the scene graph", uuidStr.c_str());
		node.release();
		return InvalidNodeId;
	}

	if (parent >= 0) {
		auto parentIter = _nodes.find(parent);
		if (parentIter == _nodes.end()) {
			Log::error("Could not find parent node with id %i", parent);
			node.release();
			return InvalidNodeId;
		}
		Log::debug("Add child %i to node %i", nodeId, parent);
		parentIter->value.addChild(nodeId);
	}
	++_nextNodeId;
	node.setId(nodeId);
	if (node.name().empty()) {
		node.setName(core::String::format("node %i", nodeId));
	}
	if (_activeNodeId == InvalidNodeId) {
		// try to set a sane default value for the active node
		if (node.isAnyModelNode()) {
			_activeNodeId = nodeId;
		}
	}
	node.setParent(parent);
	node.setAnimation(_activeAnimation);
	Log::debug("Adding scene graph node of type %i with id %i and parent %i", (int)type, node.id(), node.parent());
	_nodes.emplace(nodeId, core::forward<SceneGraphNode>(node));
	if (type == SceneGraphNodeType::Model) {
		_regionDirty = true;
	}
	for (SceneGraphListener *listener : _listeners) {
		listener->onNodeAdded(nodeId);
	}
	markMaxFramesDirty();
	return nodeId;
}

bool SceneGraph::nodeHasChildren(const SceneGraphNode &n, int childId) const {
	for (int c : n.children()) {
		if (c == childId) {
			return true;
		}
	}
	for (int c : n.children()) {
		if (nodeHasChildren(node(c), childId)) {
			return true;
		}
	}
	return false;
}

bool SceneGraph::canChangeParent(const SceneGraphNode &node, int newParentId) const {
	if (node.id() == root().id()) {
		return false;
	}
	if (!hasNode(newParentId)) {
		return false;
	}
	return !nodeHasChildren(node, newParentId);
}

bool SceneGraph::changeParent(int nodeId, int newParentId, NodeMoveFlag flag) {
	if (!hasNode(nodeId)) {
		return false;
	}
	SceneGraphNode &n = node(nodeId);
	if (!canChangeParent(n, newParentId)) {
		return false;
	}

	const int oldParentId = n.parent();
	if (!node(oldParentId).removeChild(nodeId)) {
		return false;
	}
	if (!node(newParentId).addChild(nodeId)) {
		node(oldParentId).addChild(nodeId);
		return false;
	}
	n.setParent(newParentId);
	if (flag == NodeMoveFlag::UpdateTransform) {
		for (const core::String &animation : animations()) {
			for (SceneGraphKeyFrame &keyframe : n.keyFrames(animation)) {
				SceneGraphTransform &transform = keyframe.transform();
				transform.markDirtyParent();
			}
		}
		updateTransforms();
	} else if (flag == NodeMoveFlag::KeepWorldTransform) {
		for (const core::String &animation : animations()) {
			for (SceneGraphKeyFrame &keyframe : n.keyFrames(animation)) {
				SceneGraphTransform &transform = keyframe.transform();
				const glm::mat4 worldMatrix = transform.worldMatrix();
				transform.setWorldMatrix(worldMatrix);
			}
		}
		updateTransforms();
	}
	for (SceneGraphListener *listener : _listeners) {
		listener->onNodeChangedParent(nodeId);
	}

	return true;
}

bool SceneGraph::removeNode(int nodeId, bool recursive) {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		Log::debug("Could not remove node %i - not found", nodeId);
		return false;
	}
	const SceneGraphNodeType type = iter->value.type();
	if (type == SceneGraphNodeType::Root) {
		core_assert(nodeId == 0);
		clear();
		return true;
	}
	bool state = true;
	const int parent = iter->value.parent();
	core_assert(parent != InvalidNodeId);
	SceneGraphNode &parentNode = node(parent);
	core_assert_always(parentNode.removeChild(nodeId));

	if (recursive) {
		state = iter->value.children().empty();
		for (int childId : iter->value.children()) {
			state |= removeNode(childId, recursive);
		}
	} else {
		// reparent any children
		for (int childId : iter->value.children()) {
			SceneGraphNode &cnode = node(childId);
			core_assert(cnode.parent() == nodeId);
			cnode.setParent(parent);
			core_assert_always(parentNode.addChild(childId));
		}
	}
	for (SceneGraphListener *listener : _listeners) {
		listener->onNodeRemove(nodeId);
	}
	_nodes.erase(iter);
	if (_activeNodeId == nodeId) {
		if (!empty(SceneGraphNodeType::Model)) {
			// get the first model node
			_activeNodeId = (*beginModel()).id();
		} else {
			_activeNodeId = root().id();
		}
	}
	if (type == SceneGraphNodeType::Model) {
		_regionDirty = true;
	}
	return state;
}

void SceneGraph::setAllKeyFramesForNode(SceneGraphNode &node, const SceneGraphKeyFramesMap &keyFrames) {
	node.setAllKeyFrames(keyFrames, _activeAnimation);
	visitChildren(node.id(), true, [](SceneGraphNode &child) {
		auto &nodeKeyFrames = child.allKeyFrames();
		for (const auto &entry : nodeKeyFrames) {
			for (scenegraph::SceneGraphKeyFrame &frame : entry->value) {
				frame.transform().markDirtyParent();
			}
		}
	});
}

void SceneGraph::reserve(size_t size) {
}

bool SceneGraph::empty(SceneGraphNodeType type) const {
	for (const auto &entry : _nodes) {
		if (entry->value.type() == type) {
			return false;
		}
	}
	return true;
}

size_t SceneGraph::size(SceneGraphNodeType type) const {
	if (type == SceneGraphNodeType::All) {
		return _nodes.size();
	}
	size_t n = 0;
	for (const auto &entry : _nodes) {
		if (entry->value.type() == type) {
			++n;
		} else if (type == SceneGraphNodeType::AllModels) {
			if (entry->value.isAnyModelNode()) {
				++n;
			}
		}
	}
	return n;
}

void SceneGraph::clear() {
	for (const auto &entry : _nodes) {
		entry->value.release();
	}
	_nodes.clear();
	_animations.clear();
	addAnimation(DEFAULT_ANIMATION);
	setAnimation(DEFAULT_ANIMATION);
	_nextNodeId = 1;

	SceneGraphNode node(SceneGraphNodeType::Root);
	node.setName("root");
	node.setId(0);
	node.setParent(InvalidNodeId);
	_nodes.emplace(0, core::move(node));
	_region = voxel::Region::InvalidRegion;
}

bool SceneGraph::hasMoreThanOnePalette() const {
	uint64_t hash = 0;
	for (auto entry : nodes()) {
		if (!entry->second.isAnyModelNode()) {
			continue;
		}
		const SceneGraphNode &node = entry->second;
		if (hash == 0) {
			hash = node.palette().hash();
		} else if (hash != node.palette().hash()) {
			Log::debug("Scenegraph has more than one palette");
			return true;
		}
	}
	Log::debug("Scenegraph has only one palette");
	return false;
}

bool SceneGraph::checkSamePalette() const {
	uint64_t hash = 0;
	for (const auto entry : _nodes) {
		const SceneGraphNode &node = entry->second;
		if (!node.isAnyModelNode()) {
			continue;
		}

		if (hash == 0) {
			hash = node.palette().hash();
		} else {
			if (hash != node.palette().hash()) {
				Log::debug("Palettes differ between model nodes");
				return false;
			}
		}
	}
	Log::debug("Palettes are the same for model nodes");
	return true;
}

palette::Palette SceneGraph::mergePalettes(bool removeUnused, int emptyIndex) const {
	if (checkSamePalette()) {
		return firstPalette();
	}

	palette::Palette palette;
	bool tooManyColors = false;
	for (const auto entry : _nodes) {
		const SceneGraphNode &node = entry->second;
		if (!node.isAnyModelNode()) {
			continue;
		}
		const palette::Palette &nodePalette = node.palette();
		for (int i = 0; i < nodePalette.colorCount(); ++i) {
			const core::RGBA rgba = nodePalette.color(i);
			if (palette.hasColor(rgba)) {
				continue;
			}
			uint8_t index = 0;
			int skipIndex = rgba.a == 0 ? -1 : emptyIndex;
			if (!palette.tryAdd(rgba, false, &index, false, skipIndex)) {
				if (index < palette.colorCount() - 1) {
					tooManyColors = true;
					break;
				}
			}
			if (nodePalette.hasEmit(i)) {
				palette.setEmit(index, nodePalette.material(i).emit);
			}
		}
		if (tooManyColors) {
			break;
		}
	}
	if (tooManyColors) {
		Log::debug("too many colors - restart, but skip similar");
		palette.setSize(0);
		for (int i = 0; i < palette::PaletteMaxColors; ++i) {
			palette.setMaterial(i, palette::Material{});
		}
		// TODO: SCENEGRAPH: this puts higher priority on the first model nodes
		//                   use quantization to increase the spectrum of colors
		for (const auto &e : nodes()) {
			const SceneGraphNode &node = e->second;
			if (!node.isAnyModelNode()) {
				continue;
			}
			core::Array<bool, palette::PaletteMaxColors> used;
			if (removeUnused) {
				used.fill(false);
				auto func = [&used](int, int, int, const voxel::Voxel &voxel) { used[voxel.getColor()] = true; };
				voxelutil::visitVolumeParallel(*resolveVolume(node), func);
			} else {
				used.fill(true);
			}
			const palette::Palette &nodePalette = node.palette();
			for (int i = 0; i < nodePalette.colorCount(); ++i) {
				if (!used[i]) {
					Log::trace("color %i not used, skip it for this node", i);
					continue;
				}
				uint8_t index = 0;
				const core::RGBA rgba = nodePalette.color(i);
				int skipIndex = rgba.a == 0 ? -1 : emptyIndex;
				if (palette.tryAdd(rgba, true, &index, true, skipIndex)) {
					if (nodePalette.hasEmit(i)) {
						palette.setEmit(index, nodePalette.material(i).emit);
					}
				}
			}
		}
	}
	palette.markDirty();
	return palette;
}

const palette::Palette &SceneGraph::resolvePalette(const SceneGraphNode &n) const {
	if (n.type() == SceneGraphNodeType::ModelReference) {
		return resolvePalette(node(n.reference()));
	}
	core_assert_msg(n.type() == SceneGraphNodeType::Model, "Trying to resolve palette for node of type %i", (int)n.type());
	return n.palette();
}

voxel::Region SceneGraph::resolveRegion(const SceneGraphNode &n) const {
	if (n.type() == SceneGraphNodeType::ModelReference) {
		return resolveRegion(node(n.reference()));
	}
	core_assert_msg(n.type() == SceneGraphNodeType::Model, "Trying to resolve region for node of type %i", (int)n.type());
	return n.region();
}

const voxel::RawVolume *SceneGraph::resolveVolume(const SceneGraphNode &n) const {
	if (n.type() == SceneGraphNodeType::ModelReference) {
		return resolveVolume(node(n.reference()));
	}
	core_assert_msg(n.type() == SceneGraphNodeType::Model, "Trying to resolve region for node of type %i", (int)n.type());
	return n.volume();
}

voxel::RawVolume *SceneGraph::resolveVolume(SceneGraphNode &n) {
	if (n.type() == SceneGraphNodeType::ModelReference) {
		return resolveVolume(node(n.reference()));
	}
	return n.volume();
}

SceneGraph::MergeResult SceneGraph::merge(bool skipHidden) const {
	core_trace_scoped(Merge);
	const size_t n = size(SceneGraphNodeType::AllModels);
	if (n == 0) {
		return MergeResult{};
	} else if (n == 1) {
		if (const SceneGraphNode *node = firstModelNode()) {
			if (skipHidden && !node->visible()) {
				return MergeResult{};
			}
			return MergeResult{new voxel::RawVolume(node->volume()), node->palette(), node->normalPalette()};
		}
	}

	const KeyFrameIndex keyFrameIdx = 0;
	const voxel::Region &mergedRegion = sceneRegion(keyFrameIdx, skipHidden);
	if (!mergedRegion.isValid()) {
		return MergeResult{};
	}
	const size_t bytes = voxel::RawVolume::size(mergedRegion);
	if (!app::App::getInstance()->hasEnoughMemory(bytes)) {
		Log::error("Not enough memory to merge the scene graph nodes");
		return MergeResult{};
	}
	const palette::Palette &mergedPalette = mergePalettes(true);
	const palette::NormalPalette &normalPalette = firstModelNode()->normalPalette();
	palette::PaletteLookup mergedPaletteLookup(mergedPalette);

	voxel::RawVolume *merged = new voxel::RawVolume(mergedRegion);
	int cnt = 0;
	for (const auto &e : nodes()) {
		const SceneGraphNode &node = e->second;
		if (!node.isAnyModelNode()) {
			++cnt;
			continue;
		}
		if (skipHidden && !node.visible()) {
			++cnt;
			continue;
		}
		const voxel::Region &sourceRegion = resolveRegion(node);
		const voxel::Region &destRegion = sceneRegion(node, keyFrameIdx);
		const palette::Palette &pal = node.palette();

		auto mergeCondition = [&pal, &mergedPaletteLookup](const voxel::RawVolume::Sampler &sampler) {
			voxel::Voxel voxel = sampler.voxel();
			if (isAir(voxel.getMaterial())) {
				return false;
			}
			const core::RGBA color = pal.color(voxel.getColor());
			const uint8_t index = mergedPaletteLookup.findClosestIndex(color);
			voxel.setColor(index);
			return true;
		};
		const voxel::RawVolume *v = resolveVolume(node);
		// TODO: SCENEGRAPH: scaling is not applied properly
		const glm::vec3 angles = glm::eulerAngles(node.transform(keyFrameIdx).worldOrientation());
		if (glm::all(glm::epsilonEqual(angles, glm::vec3(0.0f), 0.001f))) {
			voxelutil::mergeVolumes(merged, v, destRegion, sourceRegion, mergeCondition);
		} else {
			voxel::RawVolume *rotated = voxelutil::rotateVolume(v, angles, node.pivot());
			voxelutil::mergeVolumes(merged, rotated, destRegion, sourceRegion, mergeCondition);
			delete rotated;
		}
		Log::debug("Merged node %i/%i", cnt, (int)n);
		++cnt;
	}
	return MergeResult{merged, mergedPalette, normalPalette};
}

void SceneGraph::align(int padding) {
	core::Buffer<stbrp_rect> stbRects;
	int width = 0;
	int depth = 0;
	for (const auto &entry : nodes()) {
		const SceneGraphNode &node = entry->second;
		if (!node.isModelNode()) {
			continue;
		}
		const voxel::Region &region = node.region();
		width += region.getWidthInVoxels() + padding;
		depth += region.getDepthInVoxels() + padding;
		stbrp_rect rect;
		core_memset(&rect, 0, sizeof(rect));
		rect.id = node.id();
		rect.w = region.getWidthInVoxels() + padding;
		rect.h = region.getDepthInVoxels() + padding;
		stbRects.emplace_back(rect);
	}
	if (width == 0 || depth == 0) {
		return;
	}

	if (stbRects.size() <= 1) {
		return;
	}

	core::Buffer<stbrp_node> stbNodes;
	stbNodes.resize(width);

	stbrp_context context;
	int divisor = 16;
	for (int i = 0; i < 5; ++i) {
		core_memset(&context, 0, sizeof(context));
		stbrp_init_target(&context, width / divisor, depth / divisor, stbNodes.data(), (int)stbNodes.size());
		if (stbrp_pack_rects(&context, stbRects.data(), (int)stbRects.size()) == 1) {
			Log::debug("Used width: %i, depth: %i for packing", width / divisor, depth / divisor);
			break;
		}
		if (divisor == 1) {
			Log::warn("Could not pack rects for alignment the scene graph nodes");
			return;
		}
		divisor /= 2;
	}
	for (const stbrp_rect &rect : stbRects) {
		if (!rect.was_packed) {
			Log::warn("Failed to pack node %i", rect.id);
			continue;
		}
		SceneGraphNode &n = node(rect.id);
		SceneGraphTransform transform;
		n.setTransform(0, transform);
		n.setPivot(glm::vec3(0.0f));
		n.volume()->translate(-n.region().getLowerCorner());
		n.volume()->translate(glm::ivec3(rect.x, 0, rect.y));
	}
	for (SceneGraphListener *listener : _listeners) {
		listener->onNodesAligned();
	}
	updateTransforms();
	markDirty();
}

} // namespace scenegraph
