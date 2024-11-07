/**
 * @file
 */

#include "SceneRenderer.h"
#include "app/App.h"
#include "core/TimeProvider.h"
#include "core/Log.h"
#include "ui/Style.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneUtil.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "voxedit-util/AxisUtil.h"
#include "voxedit-util/Config.h"
#include "voxel/RawVolume.h"

namespace voxedit {

SceneRenderer::SceneRenderer() : _meshState(core::make_shared<voxel::MeshState>()) {
}

void SceneRenderer::construct() {
	_volumeRenderer.construct();
	_meshState->construct();
}

bool SceneRenderer::init() {
	_showGrid = core::Var::getSafe(cfg::VoxEditShowgrid);
	_showLockedAxis = core::Var::getSafe(cfg::VoxEditShowlockedaxis);
	_showAABB = core::Var::getSafe(cfg::VoxEditShowaabb);
	_showBones = core::Var::getSafe(cfg::VoxEditShowBones);
	_renderShadow = core::Var::getSafe(cfg::VoxEditRendershadow);
	_gridSize = core::Var::getSafe(cfg::VoxEditGridsize);
	_grayInactive = core::Var::getSafe(cfg::VoxEditGrayInactive);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
	_ambientColor = core::Var::get(cfg::VoxEditAmbientColor, "1.0 1.0 1.0");
	_diffuseColor = core::Var::get(cfg::VoxEditDiffuseColor, "0.0 0.0 0.0");

	if (!_meshState->init()) {
		Log::error("Failed to initialize the mesh state");
		return false;
	}
	if (!_volumeRenderer.init(_meshState)) {
		Log::error("Failed to initialize the volume renderer");
		return false;
	}
	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}
	if (!_gridRenderer.init()) {
		Log::error("Failed to initialize the grid renderer");
		return false;
	}

	for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
		_planeMeshIndex[i] = -1;
	}
	return true;
}

void SceneRenderer::clear() {
	_volumeRenderer.clear(_meshState);
}

void SceneRenderer::shutdown() {
	// don't free the volumes here, they belong to the scene graph
	_volumeRenderer.shutdown(_meshState);

	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();

	_aabbMeshIndex = -1;
	_boneMeshIndex = -1;
	_highlightMeshIndex = -1;
}

void SceneRenderer::updateGridRegion(const voxel::Region &region) {
	if (region.isValid()) {
		_gridRenderer.update(scenegraph::toAABB(region));
	}
}

void SceneRenderer::updateNodeRegion(int nodeId, const voxel::Region &region, uint64_t renderRegionMillis) {
	bool addNew = true;
	for (const auto &r : _extractRegions) {
		if (r.nodeId != nodeId) {
			continue;
		}
		if (r.region.containsRegion(region)) {
			addNew = false;
			break;
		}
	}
	if (addNew) {
		_extractRegions.push_back({region, nodeId});
	}
	const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
	_highlightRegion = TimedRegion(region, timeProvider->tickNow(), renderRegionMillis);
}

/**
 * @brief Return the real model node, not the reference
 */
static scenegraph::SceneGraphNode *sceneGraphModelNode(const scenegraph::SceneGraph &sceneGraph, int nodeId) {
	if (sceneGraph.hasNode(nodeId)) {
		scenegraph::SceneGraphNode *n = &sceneGraph.node(nodeId);
		if (n->reference() != InvalidNodeId) {
			if (sceneGraph.hasNode(n->reference())) {
				n = &sceneGraph.node(n->reference());
			}
		}
		return n;
	}
	return nullptr;
}

bool SceneRenderer::extractVolume(const scenegraph::SceneGraph &sceneGraph) {
	core_trace_scoped(ExtractVolume);
	const size_t n = _extractRegions.size();
	if (n <= 0) {
		return false;
	}
	Log::debug("Extract the meshes for %i regions", (int)n);
	for (size_t i = 0; i < n; ++i) {
		const voxel::Region &region = _extractRegions[i].region;
		if (scenegraph::SceneGraphNode *node = sceneGraphModelNode(sceneGraph, _extractRegions[i].nodeId)) {
			_volumeRenderer.scheduleRegionExtraction(_meshState, *node, region);
			Log::debug("Extract node %i", _extractRegions[i].nodeId);
			voxel::logRegion("Extraction", region);
		}
	}
	_extractRegions.clear();
	return true;
}

void SceneRenderer::updateLockedPlanes(math::Axis lockedAxis, const scenegraph::SceneGraph &sceneGraph,
									   const glm::ivec3 &cursorPosition) {
	if (_lockedAxis == lockedAxis) {
		return;
	}
	_lockedAxis = lockedAxis;
	updateLockedPlane(lockedAxis, math::Axis::X, sceneGraph, cursorPosition);
	updateLockedPlane(lockedAxis, math::Axis::Y, sceneGraph, cursorPosition);
	updateLockedPlane(lockedAxis, math::Axis::Z, sceneGraph, cursorPosition);
}

void SceneRenderer::updateLockedPlane(math::Axis lockedAxis, math::Axis axis, const scenegraph::SceneGraph &sceneGraph,
									  const glm::ivec3 &cursorPosition) {
	if (axis == math::Axis::None) {
		return;
	}
	const int index = math::getIndexForAxis(axis);
	int32_t &meshIndex = _planeMeshIndex[index];
	if ((lockedAxis & axis) == math::Axis::None) {
		if (meshIndex != -1) {
			_shapeRenderer.deleteMesh(meshIndex);
			meshIndex = -1;
		}
		return;
	}

	const int activeNode = sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &node = sceneGraph.node(activeNode);
	if (!node.isModelNode()) {
		if (meshIndex != -1) {
			_shapeRenderer.deleteMesh(meshIndex);
			meshIndex = -1;
		}
		return;
	}
	const glm::vec4 colors[] = {core::Color::LightRed(), core::Color::LightGreen(), core::Color::LightBlue()};
	updateShapeBuilderForPlane(_shapeBuilder, node.region(), false, cursorPosition, axis,
							   core::Color::alpha(colors[index], 0.4f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
}

void SceneRenderer::updateAABBMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph,
								   scenegraph::FrameIndex frameIdx) {
	if (!sceneMode|| !_showAABB->boolVal()) {
		return;
	}
	core_trace_scoped(UpdateAABBMesh);
	_shapeBuilder.clear();
	for (auto entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (!node.isAnyModelNode()) {
			continue;
		}
		if (!node.visible()) {
			continue;
		}
		if (node.id() == sceneGraph.activeNode()) {
			continue;
		} else if (node.isReference()) {
			_shapeBuilder.setColor(style::color(style::ColorReferenceNode));
		} else {
			_shapeBuilder.setColor(style::color(style::ColorInactiveNode));
		}
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		core_assert_msg(region.isValid(), "Region for node %s of type %i is invalid", node.name().c_str(), (int)node.type());
		const glm::vec3 pivot = node.pivot();
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
		const math::OBB<float>& obb = scenegraph::toOBB(true, region, pivot, transform);
		// TODO: make this an aabb and use the transform matrix from the rawvolumerenderer
		_shapeBuilder.obb(obb);
	}

	const scenegraph::SceneGraphNode &activeNode = sceneGraph.node(sceneGraph.activeNode());
	if (activeNode.isAnyModelNode() && activeNode.visible()) {
		_shapeBuilder.setColor(style::color(style::ColorActiveNode));
		const voxel::RawVolume *v = sceneGraph.resolveVolume(activeNode);
		core_assert(v != nullptr);
		const voxel::Region &region = v->region();
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(activeNode, frameIdx);
		// TODO: make this an aabb and use the transform matrix from the rawvolumerenderer
		_shapeBuilder.obb(scenegraph::toOBB(sceneMode, region, activeNode.pivot(), transform));
	}

	_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
}

void SceneRenderer::updateBoneMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph,
								   scenegraph::FrameIndex frameIdx) {
	if (!sceneMode) {
		return;
	}
	if (!_showBones->boolVal()) {
		return;
	}
	core_trace_scoped(UpdateBoneMesh);
	_shapeBuilder.clear();
	_shapeBuilder.setColor(style::color(style::ColorBone));

	const bool hideInactive = _hideInactive->boolVal();
	const int activeNodeId = sceneGraph.activeNode();
	for (auto entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (!node.isAnyModelNode()) {
			continue;
		}
		if (!node.visible()) {
			continue;
		}
		if (node.parent() == InvalidNodeId) {
			continue;
		}
		const bool isActiveNode = node.id() == activeNodeId;
		if (hideInactive && !isActiveNode) {
			continue;
		}

		const scenegraph::SceneGraphNode &pnode = sceneGraph.node(node.parent());
		if (!pnode.isAnyModelNode()) {
			continue;
		}
		if (!pnode.visible()) {
			continue;
		}

		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
		const scenegraph::FrameTransform &ptransform = sceneGraph.transformForFrame(pnode, frameIdx);
		const glm::vec3 &ptranslation = ptransform.translation();
		const glm::vec3 &translation = transform.translation();

		_shapeBuilder.bone(ptranslation, translation);
	}

	_shapeRenderer.createOrUpdate(_boneMeshIndex, _shapeBuilder);
}

bool SceneRenderer::isVisible(int nodeId, bool hideEmpty) const {
	return _volumeRenderer.isVisible(_meshState, nodeId, hideEmpty);
}

void SceneRenderer::removeNode(int nodeId) {
	_volumeRenderer.nodeRemove(_meshState, nodeId);
}

void SceneRenderer::update() {
	_gridRenderer.setRenderAABB(_showAABB->boolVal());
	_gridRenderer.setRenderGrid(_showGrid->boolVal());
	_gridRenderer.setGridResolution(_gridSize->intVal());
	_gridRenderer.setColor(style::color(style::ColorGridBorder));
	glm::vec3 val;
	_ambientColor->vec3Val(&val[0]);
	_volumeRenderer.setAmbientColor(val);
	_diffuseColor->vec3Val(&val[0]);
	_volumeRenderer.setDiffuseColor(val);
	_volumeRenderer.update(_meshState);
}

void SceneRenderer::renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	core_trace_scoped(RenderScene);
	if (renderContext.sceneGraph == nullptr) {
		Log::error("No scenegraph given to render");
		return;
	}

	renderContext.hideInactive = _hideInactive->boolVal();
	renderContext.grayInactive = _grayInactive->boolVal();

	video::ScopedState depthTest(video::State::DepthTest, true);
	updateAABBMesh(renderContext.renderMode == voxelrender::RenderMode::Scene, *renderContext.sceneGraph, renderContext.frame);
	updateBoneMesh(renderContext.renderMode == voxelrender::RenderMode::Scene, *renderContext.sceneGraph, renderContext.frame);
	_volumeRenderer.render(_meshState, renderContext, camera, _renderShadow->boolVal(), false);
	extractVolume(*renderContext.sceneGraph);
}

void SceneRenderer::renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	if (renderContext.sceneGraph == nullptr) {
		Log::error("No scenegraph given to render");
		return;
	}
	video::ScopedState depthTest(video::State::DepthTest, true);
	video::ScopedState blend(video::State::Blend, true);
	const scenegraph::SceneGraph &sceneGraph = *renderContext.sceneGraph;
	scenegraph::SceneGraphNode *n = sceneGraphModelNode(sceneGraph, sceneGraph.activeNode());
	if (renderContext.renderMode == voxelrender::RenderMode::Scene) {
		if (_showAABB->boolVal()) {
			_shapeRenderer.render(_aabbMeshIndex, camera);
		}
		if (_showBones->boolVal()) {
			video::ScopedState depthDepthTest(video::State::DepthTest, false);
			_shapeRenderer.render(_boneMeshIndex, camera);
		}
		// TODO: allow to render a grid in scene mode - makes shifting a lot easier
		// TODO: render arrows for the distance of the region mins to the origin - to indicate a shifted region
	} else if (n != nullptr) {
		const voxel::Region &region = n->region();
		_gridRenderer.render(camera, scenegraph::toAABB(region));

		if (_showLockedAxis->boolVal()) {
			for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
				// TODO: fix z-fighting
				_shapeRenderer.render(_planeMeshIndex[i], camera);
			}
		}
	}

	const core::TimeProviderPtr &timeProvider = app::App::getInstance()->timeProvider();
	const uint64_t highlightMillis = _highlightRegion.remaining(timeProvider->tickNow());
	if (highlightMillis > 0) {
		video::ScopedPolygonMode o(video::PolygonMode::Solid, glm::vec2(1.0f, 1.0f));
		_shapeBuilder.clear();
		_shapeBuilder.setColor(style::color(style::ColorHighlightArea));
		_shapeBuilder.cube(_highlightRegion.value().getLowerCornerf(),
						   _highlightRegion.value().getUpperCornerf() + 1.0f);
		_shapeRenderer.createOrUpdate(_highlightMeshIndex, _shapeBuilder);
		_shapeRenderer.render(_highlightMeshIndex, camera);
		video::polygonOffset(glm::vec2(0.0f));
	}
}

} // namespace voxedit
