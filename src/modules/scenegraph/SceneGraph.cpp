/**
 * @file
 */

#include "SceneGraph.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeVisitor.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>
#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_ASSERT core_assert
#include "external/stb_rect_pack.h"

namespace scenegraph {

glm::mat4 FrameTransform::worldMatrix() const {
	return glm::translate(translation) * glm::mat4_cast(orientation) * glm::scale(scale);
}

SceneGraph::SceneGraph(int nodes) : _nodes(nodes), _activeAnimation(DEFAULT_ANIMATION) {
	clear();
	_animations.push_back(_activeAnimation);
}

SceneGraph::~SceneGraph() {
	for (const auto &entry : _nodes) {
		entry->value.release();
	}
	_nodes.clear();
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

bool SceneGraph::setAnimation(const core::String &animation) {
	if (animation.empty()) {
		return false;
	}
	if (core::find(_animations.begin(), _animations.end(), animation) == _animations.end()) {
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
	if (core::find(_animations.begin(), _animations.end(), animation) == _animations.end()) {
		Log::error("Could not find animation %s", animation.c_str());
		return false;
	}
	if (core::find(_animations.begin(), _animations.end(), newName) != _animations.end()) {
		Log::error("Animation %s already exists", newName.c_str());
		return false;
	}
	_animations.push_back(newName);
	for (const auto &entry : _nodes) {
		SceneGraphNode &node = entry->value;
		if (!node.duplicateKeyFrames(animation, newName)) {
			Log::warn("Failed to set keyframes for node %i and animation %s", node.id(), animation.c_str());
		}
	}
	updateTransforms_r(node(0));
	return true;
}

bool SceneGraph::addAnimation(const core::String &animation) {
	if (animation.empty()) {
		return false;
	}
	if (core::find(_animations.begin(), _animations.end(), animation) != _animations.end()) {
		return false;
	}
	_animations.push_back(animation);
	return true;
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

FrameIndex SceneGraph::maxFrames(const core::String &animation) const {
	if (_cachedMaxFrame == -1) {
		for (const auto &entry : nodes()) {
			const SceneGraphNode &node = entry->second;
			if (node.allKeyFrames().empty()) {
				continue;
			}
			const FrameIndex maxFrame = node.maxFrame(animation);
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
	return _nodes.find(nodeId) != _nodes.end();
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
	const auto &children = node(ownNode.parent()).children();
	for (int child : children) {
		if (child == nodeId) {
			continue;
		}
		if (node(child).isAnyModelNode()) {
			return child;
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

AnimState SceneGraph::transformFrameSource_r(const SceneGraphNode &node, const core::String &animation,
											 FrameIndex frameIdx) const {
	const SceneGraphKeyFrames &keyFrames = node.keyFrames(animation);
	const SceneGraphKeyFrame *match = nullptr;
	AnimState state;
	core_assert(frameIdx >= 0);
	core_assert(node.keyFramesValidate());
	for (const SceneGraphKeyFrame &kf : keyFrames) {
		if (kf.frameIdx == frameIdx) {
			match = &kf;
			break;
		}
	}
	if (match != nullptr) {
		const SceneGraphTransform &transform = match->transform();
		state.transform.orientation = transform.worldOrientation();
		state.transform.translation = transform.worldTranslation();
		state.transform.scale = transform.worldScale();
		state.frameIdx = match->frameIdx;
		state.interpolation = match->interpolation;
		state.longRotation = match->longRotation;
		return state;
	}
	const KeyFrameIndex bestFit = node.keyFrameForFrame(frameIdx);
	const SceneGraphKeyFrame &kf = keyFrames[bestFit];
	const SceneGraphTransform &transform = kf.transform();
	if (node.parent() == InvalidNodeId) {
		state.transform.orientation = transform.worldOrientation();
		state.transform.translation = transform.worldTranslation();
		state.transform.scale = transform.worldScale();
		state.frameIdx = kf.frameIdx;
		state.interpolation = kf.interpolation;
		state.longRotation = kf.longRotation;
		return state;
	}
	state = transformFrameSource_r(this->node(node.parent()), animation, frameIdx);
	state.transform.orientation *= transform.localOrientation();
	state.transform.translation += transform.localTranslation();
	state.transform.scale *= transform.localScale();
	return state;
}

AnimState SceneGraph::transformFrameTarget_r(const SceneGraphNode &node, const core::String &animation,
											 FrameIndex frameIdx) const {
	const SceneGraphKeyFrames &keyFrames = node.keyFrames(animation);
	const SceneGraphKeyFrame *last = nullptr;
	AnimState state;
	core_assert(frameIdx >= 0);
	core_assert(node.keyFramesValidate());
	for (const SceneGraphKeyFrame &kf : keyFrames) {
		if (kf.frameIdx <= frameIdx) {
			last = &kf;
			continue;
		}
		const SceneGraphTransform &transform = kf.transform();
		state.transform.orientation = transform.worldOrientation();
		state.transform.translation = transform.worldTranslation();
		state.transform.scale = transform.worldScale();
		state.frameIdx = kf.frameIdx;
		state.interpolation = kf.interpolation;
		state.longRotation = kf.longRotation;
		return state;
	}
	if (node.parent() == InvalidNodeId) {
		const KeyFrameIndex bestFit = node.nextKeyFrameForFrame(frameIdx);
		const SceneGraphKeyFrame &kf = keyFrames[bestFit];
		const SceneGraphTransform &transform = kf.transform();
		state.transform.orientation = transform.worldOrientation();
		state.transform.translation = transform.worldTranslation();
		state.transform.scale = transform.worldScale();
		state.frameIdx = kf.frameIdx;
		state.interpolation = kf.interpolation;
		state.longRotation = kf.longRotation;
		return state;
	}
	core_assert(last != nullptr);
	const SceneGraphNode &parentNode = this->node(node.parent());
	const SceneGraphTransform &transform = last->transform();
	state = transformFrameTarget_r(parentNode, animation, frameIdx);
	state.transform.orientation *= transform.localOrientation();
	state.transform.translation += transform.localTranslation();
	state.transform.scale *= transform.localScale();
	return state;
}

FrameTransform SceneGraph::transformForFrame(const SceneGraphNode &node, FrameIndex frameIdx) const {
	return transformForFrame(node, _activeAnimation, frameIdx);
}

FrameTransform SceneGraph::transformForFrame(const SceneGraphNode &node, const core::String &animation,
											 FrameIndex frameIdx) const {
	// TODO ik solver https://github.com/vengi-voxel/vengi/issues/182
	const AnimState source = transformFrameSource_r(node, animation, frameIdx);
	const AnimState target = transformFrameTarget_r(node, animation, frameIdx);
	const FrameIndex startFrameIdx = source.frameIdx;
	const InterpolationType interpolationType = source.interpolation;
	const FrameIndex endFrameIdx = target.frameIdx;
	const double deltaFrameSeconds =
		scenegraph::interpolate(interpolationType, (double)frameIdx, (double)startFrameIdx, (double)endFrameIdx);
	const float factor = glm::clamp((float)(deltaFrameSeconds), 0.0f, 1.0f);

	FrameTransform transform;
	transform.translation = glm::mix(source.transform.translation, target.transform.translation, factor);
	transform.orientation = glm::slerp(source.transform.orientation, target.transform.orientation, factor);
	transform.scale = glm::mix(source.transform.scale, target.transform.scale, factor);
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

voxel::Region SceneGraph::region() const {
	voxel::Region r;
	bool validVolume = false;
	for (auto iter = beginModel(); iter != end(); ++iter) {
		const SceneGraphNode &node = *iter;
		if (validVolume) {
			r.accumulate(node.region());
			continue;
		}
		r = node.region();
		validVolume = true;
	}
	return r;
}

voxel::Region SceneGraph::sceneRegion(KeyFrameIndex keyFrameIdx) const {
	voxel::Region r;
	bool validVolume = false;
	for (auto iter = begin(SceneGraphNodeType::AllModels); iter != end(); ++iter) {
		const SceneGraphNode &node = *iter;
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

voxel::Region SceneGraph::sceneRegion(const SceneGraphNode &node, KeyFrameIndex keyFrameIdx) const {
	return node.sceneRegion(resolveRegion(node), resolvePivot(node), keyFrameIdx);
}

void SceneGraph::fixErrors() {
	Log::warn("Attempt to fix errors in the scene graph");
	for (const auto &entry : _nodes) {
		entry->value.fixErrors();
	}
	updateTransforms();
}

bool SceneGraph::validate() const {
	for (const auto &entry : _nodes) {
		if (!entry->value.validate()) {
			return false;
		}
	}
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

int SceneGraph::emplace(SceneGraphNode &&node, int parent) {
	core_assert_msg((int)node.type() < (int)SceneGraphNodeType::Max, "%i", (int)node.type());
	if (node.type() == SceneGraphNodeType::Root && _nextNodeId != 0) {
		Log::error("No second root node is allowed in the scene graph");
		node.release();
		return InvalidNodeId;
	}
	if (node.type() == SceneGraphNodeType::Model) {
		core_assert(node.volume() != nullptr);
		core_assert(node.region().isValid());
	}
	const int nodeId = _nextNodeId;
	if (parent >= nodeId) {
		Log::error("Invalid parent id given: %i", parent);
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
		node.setName(core::string::format("node %i", nodeId));
	}
	if (_activeNodeId == InvalidNodeId) {
		// try to set a sane default value for the active node
		if (node.isAnyModelNode()) {
			_activeNodeId = nodeId;
		}
	}
	node.setParent(parent);
	node.setAnimation(_activeAnimation);
	Log::debug("Adding scene graph node of type %i with id %i and parent %i", (int)node.type(), node.id(),
			   node.parent());
	_nodes.emplace(nodeId, core::forward<SceneGraphNode>(node));
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

bool SceneGraph::changeParent(int nodeId, int newParentId, bool updateTransform) {
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
	if (updateTransform) {
		const SceneGraphNode &parentNode = node(newParentId);
		for (const core::String &animation : animations()) {
			for (SceneGraphKeyFrame &keyframe : n.keyFrames(animation)) {
				SceneGraphTransform &transform = keyframe.transform();
				const FrameTransform &parentFrameTransform =
					transformForFrame(parentNode, animation, keyframe.frameIdx);
				const glm::vec3 &tdelta = transform.worldTranslation() - parentFrameTransform.translation;
				const glm::quat &tquat = transform.worldOrientation() - parentFrameTransform.orientation;
				transform.setLocalTranslation(tdelta);
				transform.setLocalOrientation(tquat);
			}
		}
		updateTransforms();
	}
	return true;
}

bool SceneGraph::removeNode(int nodeId, bool recursive) {
	auto iter = _nodes.find(nodeId);
	if (iter == _nodes.end()) {
		Log::debug("Could not remove node %i - not found", nodeId);
		return false;
	}
	if (iter->value.type() == SceneGraphNodeType::Root) {
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
	core_assert_always(_nodes.erase(iter));
	if (_activeNodeId == nodeId) {
		if (!empty(SceneGraphNodeType::Model)) {
			// get the first model node
			_activeNodeId = (*beginModel()).id();
		} else {
			_activeNodeId = root().id();
		}
	}
	return state;
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
	_nextNodeId = 1;

	SceneGraphNode node(SceneGraphNodeType::Root);
	node.setName("root");
	node.setId(0);
	node.setParent(InvalidNodeId);
	_nodes.emplace(0, core::move(node));
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

palette::Palette SceneGraph::mergePalettes(bool removeUnused, int emptyIndex) const {
	palette::Palette palette;
	bool tooManyColors = false;
	for (auto iter = beginAllModels(); iter != end(); ++iter) {
		const SceneGraphNode &node = *iter;
		const palette::Palette &nodePalette = node.palette();
		for (int i = 0; i < nodePalette.colorCount(); ++i) {
			const core::RGBA rgba = nodePalette.color(i);
			if (palette.hasColor(rgba)) {
				continue;
			}
			uint8_t index = 0;
			int skipIndex = rgba.a == 0 ? -1 : emptyIndex;
			if (!palette.addColorToPalette(rgba, false, &index, false, skipIndex)) {
				if (index < palette.colorCount() - 1) {
					tooManyColors = true;
					break;
				}
			}
			if (nodePalette.hasGlow(i)) {
				palette.setGlow(index, 1.0f);
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
			palette.removeGlow(i);
		}
		for (auto iter = beginAllModels(); iter != end(); ++iter) {
			const SceneGraphNode &node = *iter;
			core::Array<bool, palette::PaletteMaxColors> used;
			if (removeUnused) {
				used.fill(false);
				voxelutil::visitVolume(*node.volume(), [&used](int, int, int, const voxel::Voxel &voxel) {
					used[voxel.getColor()] = true;
				});
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
				if (palette.addColorToPalette(rgba, true, &index, true, skipIndex)) {
					if (nodePalette.hasGlow(i)) {
						palette.setGlow(index, 1.0f);
					}
				}
			}
		}
	}
	palette.markDirty();
	return palette;
}

voxel::Region SceneGraph::resolveRegion(const SceneGraphNode &n) const {
	if (n.type() == SceneGraphNodeType::ModelReference) {
		return resolveRegion(node(n.reference()));
	}
	return n.region();
}

glm::vec3 SceneGraph::resolvePivot(const SceneGraphNode &n) const {
	if (n.type() == SceneGraphNodeType::ModelReference) {
		return resolvePivot(node(n.reference()));
	}
	return n.pivot();
}

voxel::RawVolume *SceneGraph::resolveVolume(const SceneGraphNode &n) const {
	if (n.type() == SceneGraphNodeType::ModelReference) {
		return resolveVolume(node(n.reference()));
	}
	return n.volume();
}

SceneGraph::MergedVolumePalette SceneGraph::merge(bool applyTransform, bool skipHidden) const {
	const size_t n = size(SceneGraphNodeType::AllModels);
	if (n == 0) {
		return MergedVolumePalette{};
	} else if (n == 1) {
		auto iter = beginModel();
		if (iter != end()) {
			const SceneGraphNode &node = *iter;
			if (skipHidden && !node.visible()) {
				return MergedVolumePalette{};
			}
			return MergedVolumePalette{new voxel::RawVolume(node.volume()), node.palette()};
		}
	}

	core::DynamicArray<const SceneGraphNode *> nodes;
	nodes.reserve(n);

	voxel::Region mergedRegion = voxel::Region::InvalidRegion;
	const palette::Palette &palette = mergePalettes(true);
	const KeyFrameIndex keyFrameIdx = 0;

	for (auto iter = begin(SceneGraphNodeType::AllModels); iter != end(); ++iter) {
		const SceneGraphNode &node = *iter;
		if (skipHidden && !node.visible()) {
			continue;
		}
		nodes.push_back(&node);

		voxel::Region region = resolveRegion(node);
		if (applyTransform) {
			const SceneGraphTransform &transform = node.transform(keyFrameIdx);
			const glm::vec3 &translation = transform.worldTranslation();
			region.shift(translation);
		}
		if (mergedRegion.isValid()) {
			mergedRegion.accumulate(region);
		} else {
			mergedRegion = region;
		}
	}

	voxel::RawVolume *merged = new voxel::RawVolume(mergedRegion);
	for (size_t i = 0; i < nodes.size(); ++i) {
		const SceneGraphNode *node = nodes[i];
		const voxel::Region &sourceRegion = resolveRegion(*node);
		voxel::Region destRegion = sourceRegion;
		if (applyTransform) {
			const SceneGraphTransform &transform = node->transform(keyFrameIdx);
			const glm::vec3 &translation = transform.worldTranslation();
			destRegion.shift(translation);
			// TODO: rotation
		}

		voxelutil::mergeVolumes(merged, resolveVolume(*node), destRegion, sourceRegion,
								[node, &palette](voxel::Voxel &voxel) {
									if (isAir(voxel.getMaterial())) {
										return false;
									}
									const core::RGBA color = node->palette().color(voxel.getColor());
									const uint8_t index = palette.getClosestMatch(color);
									voxel.setColor(index);
									return true;
								});
	}
	return MergedVolumePalette{merged, palette};
}

void SceneGraph::align(int padding) {
	core::DynamicArray<stbrp_rect> stbRects;
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

	core::DynamicArray<stbrp_node> stbNodes;
	stbNodes.resize(width);

	stbrp_context context;
	int divisor = 16;
	for (int i = 0; i < 5; ++i) {
		core_memset(&context, 0, sizeof(context));
		stbrp_init_target(&context, width / divisor, depth / divisor, stbNodes.data(), stbNodes.size());
		if (stbrp_pack_rects(&context, stbRects.data(), stbRects.size()) == 1) {
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
	updateTransforms();
	markDirty();
}

} // namespace scenegraph
