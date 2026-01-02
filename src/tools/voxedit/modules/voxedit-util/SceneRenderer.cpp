/**
 * @file
 */

#include "SceneRenderer.h"
#include "app/App.h"
#include "app/I18N.h"
#include "core/TimeProvider.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "scenegraph/FrameTransform.h"
#include "ui/Style.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneUtil.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "voxedit-util/AxisUtil.h"
#include "voxedit-util/Config.h"
#include "voxel/RawVolume.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/SceneGraphRenderer.h"

namespace voxedit {

SceneRenderer::SceneRenderer() : _meshState(core::make_shared<voxel::MeshState>()) {
}

void SceneRenderer::construct() {
	_sceneGraphRenderer.construct();
	_meshState->construct();
}

bool SceneRenderer::init() {
	_showGrid = core::Var::getSafe(cfg::VoxEditShowgrid);
	_showLockedAxis = core::Var::getSafe(cfg::VoxEditShowlockedaxis);
	_showAABB = core::Var::getSafe(cfg::VoxEditShowaabb);
	_showBones = core::Var::getSafe(cfg::VoxEditShowBones);
	_renderShadow = core::Var::getSafe(cfg::VoxEditRendershadow);
	_shadingMode = core::Var::get(cfg::VoxEditShadingMode, "1", _("Shading mode: 0=Unlit, 1=Lit, 2=Shadows"));
	_gridSize = core::Var::getSafe(cfg::VoxEditGridsize);
	_grayInactive = core::Var::getSafe(cfg::VoxEditGrayInactive);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
	_ambientColor = core::Var::get(cfg::VoxEditAmbientColor, "0.3 0.3 0.3");
	_diffuseColor = core::Var::get(cfg::VoxEditDiffuseColor, "0.7 0.7 0.7");
	_sunAngle = core::Var::get(cfg::VoxEditSunAngle, "35.0 135.0 0.0", _("pitch, yaw and ignored roll in degrees"));
	_planeSize = core::Var::getSafe(cfg::VoxEditPlaneSize);
	_showPlane = core::Var::getSafe(cfg::VoxEditShowPlane);

	if (!_meshState->init()) {
		Log::error("Failed to initialize the mesh state");
		return false;
	}
	if (!_sceneGraphRenderer.init(_meshState->hasNormals())) {
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
	_sceneGraphRenderer.clear(_meshState);
}

const voxel::Region &SceneRenderer::sliceRegion() const {
	return _sceneGraphRenderer.sliceRegion();
}

void SceneRenderer::setSliceRegion(const voxel::Region &region) {
	_sceneGraphRenderer.setSliceRegion(region);
}

bool SceneRenderer::isSliceModeActive() const {
	return _sceneGraphRenderer.isSliceModeActive();
}

SceneRenderer::RendererStats SceneRenderer::rendererStats() const {
	RendererStats stats;
	stats.pendingExtractions = _meshState->pendingExtractions();
	stats.pendingMeshes = _meshState->pendingMeshes();
	stats.culledVolumes = _sceneGraphRenderer.culledVolumeCount();
	return stats;
}

void SceneRenderer::shutdown() {
	_sceneGraphRenderer.shutdown();
	// don't free the volumes here, they belong to the scene graph
	(void)_meshState->shutdown();

	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();

	_sliceRegionMeshIndex = -1;
	_aabbMeshIndex = -1;
	_boneMeshIndex = -1;
	_highlightMeshIndex = -1;
}

void SceneRenderer::updateGridRegion(const voxel::Region &region) {
	const math::AABB<float> &aabb = scenegraph::toAABB(region);
	_gridRenderer.update(aabb);
}

void SceneRenderer::updateNodeRegion(int nodeId, const voxel::Region &region, uint64_t renderRegionMillis) {
	_sceneGraphRenderer.scheduleRegionExtraction(_meshState, nodeId, region);
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
	glm::vec4 color{0.0f};
	if (axis == math::Axis::X) {
		color = style::color(style::ColorAxisX);
	} else if (axis == math::Axis::Y) {
		color = style::color(style::ColorAxisY);
	} else if (axis == math::Axis::Z) {
		color = style::color(style::ColorAxisZ);
	}
	updateShapeBuilderForPlane(_shapeBuilder, node.region(), false, cursorPosition, axis,
							   color::alpha(color, 0.4f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
}

void SceneRenderer::updateAABBMesh(bool sceneMode, const scenegraph::SceneGraph &sceneGraph,
								   scenegraph::FrameIndex frameIdx) {
	if (!sceneMode || !_showAABB->boolVal()) {
		return;
	}
	core_trace_scoped(UpdateAABBMesh);
	_shapeBuilder.clear();
	const scenegraph::SceneGraphNode &activeNode = sceneGraph.node(sceneGraph.activeNode());
	const bool activeNodeLocked = activeNode.locked();
	int modelNodes = 0;
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
		} else if (activeNodeLocked && node.locked()) {
			_shapeBuilder.setColor(style::color(style::ColorLockedNode));
		} else if (node.isReferenceNode()) {
			_shapeBuilder.setColor(style::color(style::ColorReferenceNode));
		} else {
			_shapeBuilder.setColor(style::color(style::ColorInactiveNode));
		}
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		core_assert_msg(region.isValid(), "Region for node %s of type %i is invalid", node.name().c_str(), (int)node.type());
		const glm::vec3 pivot = node.pivot();
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
		const math::OBBF& obb = scenegraph::toOBB(true, region, pivot, transform);
		_shapeBuilder.obb(obb);
		++modelNodes;
	}

	if (activeNode.isAnyModelNode() && activeNode.visible()) {
		_shapeBuilder.setColor(style::color(style::ColorActiveNode));
		const voxel::RawVolume *v = sceneGraph.resolveVolume(activeNode);
		core_assert(v != nullptr);
		const voxel::Region &region = v->region();
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(activeNode, frameIdx);
		_shapeBuilder.obb(scenegraph::toOBB(sceneMode, region, activeNode.pivot(), transform));
	}

	if (modelNodes > 1 && !activeNode.children().empty()) {
		const math::AABB<float> &aabb = sceneGraph.calculateGroupAABB(activeNode, frameIdx);
		if (aabb.isValid()) {
			_shapeBuilder.setColor(style::color(style::ColorGroupNode));
			_shapeBuilder.aabb(aabb);
		}
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
		const glm::vec3 &ptranslation = ptransform.worldTranslation();
		const glm::vec3 &translation = transform.worldTranslation();

		_shapeBuilder.bone(ptranslation, translation);
	}

	_shapeRenderer.createOrUpdate(_boneMeshIndex, _shapeBuilder);
}

const voxel::RawVolume *SceneRenderer::volumeForNode(const scenegraph::SceneGraphNode &node) {
	int idx = voxelrender::SceneGraphRenderer::getVolumeIdx(node);
	const voxel::RawVolume *v = _meshState->volume(idx);
	if (v == nullptr) {
		v = node.volume();
	}
	return v;
}

bool SceneRenderer::isVisible(int nodeId, bool hideEmpty) const {
	return _sceneGraphRenderer.isVisible(_meshState, nodeId, hideEmpty);
}

void SceneRenderer::removeNode(int nodeId) {
	_sceneGraphRenderer.nodeRemove(_meshState, nodeId);
}

void SceneRenderer::update() {
	core_trace_scoped(SceneRendererUpdate);
	_gridRenderer.setRenderAABB(_showAABB->boolVal());
	_gridRenderer.setRenderGrid(_showGrid->boolVal());
	_gridRenderer.setGridResolution(_gridSize->intVal());
	_gridRenderer.setPlaneGridSize(_planeSize->intVal());
	_gridRenderer.setRenderPlane(_showPlane->boolVal());
	_gridRenderer.setColor(style::color(style::ColorGridBorder));
	glm::vec3 val;
	_ambientColor->vec3Val(&val[0]);
	_sceneGraphRenderer.setAmbientColor(val);
	_diffuseColor->vec3Val(&val[0]);
	_sceneGraphRenderer.setDiffuseColor(val);
	_sunAngle->vec3Val(&val[0]);
	_sceneGraphRenderer.setSunAngle(val);
	_sceneGraphRenderer.update(_meshState);
}

void SceneRenderer::updateSliceRegionMesh() {
	if (!isSliceModeActive()) {
		return;
	}
	core_trace_scoped(UpdateSliceRegionMesh);
	_shapeBuilder.clear();

	const math::AABB<float> &aabb = scenegraph::toAABB(sliceRegion());
	_shapeBuilder.setColor(style::color(style::ColorSliceRegion));
	_shapeBuilder.aabb(aabb);
	_shapeRenderer.createOrUpdate(_sliceRegionMeshIndex, _shapeBuilder);
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
	updateSliceRegionMesh();
	updateAABBMesh(renderContext.isSceneMode(), *renderContext.sceneGraph, renderContext.frame);
	updateBoneMesh(renderContext.isSceneMode(), *renderContext.sceneGraph, renderContext.frame);
	_sceneGraphRenderer.render(_meshState, renderContext, camera, _renderShadow->boolVal(), false);
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
	_gridRenderer.renderForwardArrow(camera);
	_gridRenderer.renderPlane(camera);
	if (renderContext.isSceneMode()) {
		if (_showAABB->boolVal()) {
			_shapeRenderer.render(_aabbMeshIndex, camera);
		}
		if (_showBones->boolVal()) {
			video::ScopedState depthDepthTest(video::State::DepthTest, false);
			_shapeRenderer.render(_boneMeshIndex, camera);
		}
		// TODO: allow to render a grid in scene mode - makes shifting a lot easier
		// TODO: render arrows for the distance of the region mins to the origin - to indicate a shifted region

		if (isSliceModeActive()) {
			// TODO: model matrix for the slice region
			_shapeRenderer.render(_sliceRegionMeshIndex, camera);
		}
	} else if (n != nullptr) {
		const voxel::Region &region = n->region();
		const bool applyTransforms = region.isValid() && renderContext.applyTransforms();
		const glm::mat4 &model = sceneGraph.worldMatrix(*n, renderContext.frame, applyTransforms);
		_gridRenderer.render(camera, scenegraph::toAABB(region), model);

		if (_showLockedAxis->boolVal()) {
			for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
				// TODO: fix z-fighting
				_shapeRenderer.render(_planeMeshIndex[i], camera, model);
			}
		}

		if (isSliceModeActive()) {
			_shapeRenderer.render(_sliceRegionMeshIndex, camera, model);
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
			_shapeRenderer.render(_highlightMeshIndex, camera, model);
			video::polygonOffset(glm::vec2(0.0f));
		}
	}
}

} // namespace voxedit
