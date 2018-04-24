/**
 * @file
 */

#include "OctreeRenderer.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedPolygonMode.h"
#include "core/Trace.h"
#include "core/Array.h"

const std::string MaxDepthBufferUniformName = "u_cascades";

namespace voxelrender {

OctreeRenderer::RenderOctreeNode::RenderOctreeNode(const video::Shader& shader) {
	for (uint8_t z = 0u; z < 2u; ++z) {
		for (uint8_t y = 0u; y < 2u; ++y) {
			for (uint8_t x = 0u; x < 2u; ++x) {
				_children[x][y][z] = nullptr;
			}
		}
	}

	_vertexBuffer = _vb.create();
	_indexBuffer = _vb.create(nullptr, 0, video::VertexBufferType::IndexBuffer);

	const int locationPos = shader.enableVertexAttributeArray("a_pos");
	const video::Attribute& posAttrib = getPositionVertexAttribute(_vertexBuffer, locationPos, shader.getAttributeComponents(locationPos));
	_vb.addAttribute(posAttrib);

	const int locationInfo = shader.enableVertexAttributeArray("a_info");
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_vertexBuffer, locationInfo, shader.getAttributeComponents(locationInfo));
	_vb.addAttribute(infoAttrib);
}

OctreeRenderer::RenderOctreeNode::~RenderOctreeNode() {
	for (uint8_t z = 0u; z < 2u; ++z) {
		for (uint8_t y = 0u; y < 2u; ++y) {
			for (uint8_t x = 0u; x < 2u; ++x) {
				delete _children[x][y][z];
				_children[x][y][z] = nullptr;
			}
		}
	}
	_vb.shutdown();
}

void OctreeRenderer::processOctreeNodeStructure(voxel::OctreeNode* octreeNode, RenderOctreeNode* node) {
	if (octreeNode->_nodeOrChildrenLastChanged <= node->_nodeAndChildrenLastSynced) {
		return;
	}

	voxel::Octree* octree = octreeNode->_octree;
	if (octreeNode->_propertiesLastChanged > node->_propertiesLastSynced) {
		Log::debug("Resynced properties at %u", node->_propertiesLastSynced);
		node->_renderThisNode = octreeNode->renderThisNode();
		node->_propertiesLastSynced = octree->time();
	}

	if (octreeNode->_meshLastChanged > node->_meshLastSynced) {
		const voxel::Mesh* mesh = octreeNode->getMesh();
		// TODO: handle water mesh properly
		const voxel::Mesh* waterMesh = octreeNode->getWaterMesh();
		if (mesh != nullptr) {
			glm::ivec3 mins(std::numeric_limits<int>::max());
			glm::ivec3 maxs(std::numeric_limits<int>::min());

			for (auto& v : mesh->getVertexVector()) {
				mins = glm::min(mins, v.position);
				maxs = glm::max(maxs, v.position);
			}
			for (auto& v : waterMesh->getVertexVector()) {
				mins = glm::min(mins, v.position);
				maxs = glm::max(maxs, v.position);
			}

			node->_aabb = math::AABB<float>(mins, maxs);
			node->_vb.update(node->_vertexBuffer, mesh->getVertexVector());
			node->_vb.update(node->_indexBuffer, mesh->getIndexVector());
		}

		node->_meshLastSynced = octree->time();
		Log::debug("Resynced mesh at %u", node->_meshLastSynced);
	}

	if (octreeNode->_structureLastChanged > node->_structureLastSynced) {
		for (uint8_t z = 0u; z < 2u; ++z) {
			for (uint8_t y = 0u; y < 2u; ++y) {
				for (uint8_t x = 0u; x < 2u; ++x) {
					RenderOctreeNode** renderNode = &(node->_children[x][y][z]);
					if (octreeNode->getChildNode(x, y, z) != nullptr) {
						if (*renderNode == nullptr) {
							// TODO: pool this
							*renderNode = new RenderOctreeNode(_worldShader);
						}
					} else {
						delete *renderNode;
						*renderNode = nullptr;
					}
				}
			}
		}

		node->_structureLastSynced = octree->time();
		Log::debug("Resynced structure at %u", node->_structureLastSynced);
	}

	octreeNode->visitExistingChildren([=] (uint8_t x, uint8_t y, uint8_t z, voxel::OctreeNode* c) {
		processOctreeNodeStructure(c, node->_children[x][y][z]);
	});
	node->_nodeAndChildrenLastSynced = octree->time();
}

void OctreeRenderer::renderOctreeNode(const video::Camera& camera, RenderOctreeNode* renderNode) {
	const int numIndices = renderNode->_vb.elements(renderNode->_indexBuffer, 1, sizeof(voxel::IndexType));
	if (numIndices > 0 && renderNode->_renderThisNode) {
		if (camera.isVisible(renderNode->_aabb)) {
			video::ScopedVertexBuffer scopedBuf(renderNode->_vb);
			video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
		}
	}

	for (uint8_t z = 0u; z < 2u; ++z) {
		for (uint8_t y = 0u; y < 2u; ++y) {
			for (uint8_t x = 0u; x < 2u; ++x) {
				RenderOctreeNode* renderChildNode = renderNode->_children[x][y][z];
				if (renderChildNode == nullptr) {
					continue;
				}
				renderOctreeNode(camera, renderChildNode);
			}
		}
	}
}

void OctreeRenderer::render(const video::Camera& camera) {
	core_trace_scoped(OctreeRendererRender);
	voxel::OctreeNode* rootNode = _volume->rootNode();
	if (rootNode != nullptr) {
		processOctreeNodeStructure(rootNode, _rootNode);
	}

	core_trace_gl_scoped(OctreeRendererTraverseOctreeTree);

	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);

	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();
	video::disable(video::State::Blend);
	// put shadow acne into the dark
	video::cullFace(video::Face::Front);
	const float shadowBiasSlope = 2;
	const float shadowBias = 0.09f;
	const float shadowRangeZ = camera.farPlane() * 3.0f;
	const glm::vec2 offset(shadowBiasSlope, (shadowBias / shadowRangeZ) * (1 << 24));
	const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

	{
		video::ScopedShader scoped(_shadowMapShader);
		_depthBuffer.bind();
		for (int i = 0; i < maxDepthBuffers; ++i) {
			_depthBuffer.bindTexture(i);
			_shadowMapShader.setLightviewprojection(cascades[i]);
			_shadowMapShader.setModel(glm::mat4(1.0f));
			renderOctreeNode(camera, _rootNode);
		}
		_depthBuffer.unbind();
	}
	video::cullFace(video::Face::Back);
	video::enable(video::State::Blend);
	_colorTexture.bind(video::TextureUnit::Zero);
	video::clearColor(_clearColor);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);
	video::bindTexture(video::TextureUnit::One, _depthBuffer);
	video::ScopedShader scoped(_worldShader);
	_worldShader.setMaterialblock(_materialBlock);
	_worldShader.setViewdistance(camera.farPlane());
	_worldShader.setLightdir(_shadow.sunDirection());
	_worldShader.setFogcolor(_clearColor);
	_worldShader.setTexture(video::TextureUnit::Zero);
	_worldShader.setDiffuseColor(_diffuseColor);
	_worldShader.setAmbientColor(_ambientColor);
	_worldShader.setFogrange(_fogRange);
	_worldShader.setViewprojection(camera.viewProjectionMatrix());
	_worldShader.setShadowmap(video::TextureUnit::One);
	_worldShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
	_worldShader.setModel(glm::mat4(1.0f));
	_worldShader.setCascades(cascades);
	_worldShader.setDistances(distances);
	renderOctreeNode(camera, _rootNode);

	_colorTexture.unbind();
}

bool OctreeRenderer::init(voxel::PagedVolume* volume, const voxel::Region& region, int baseNodeSize) {
	if (!_worldShader.setup()) {
		return false;
	}
	if (!_worldInstancedShader.setup()) {
		return false;
	}
	if (!_shadowMapInstancedShader.setup()) {
		return false;
	}
	if (!_waterShader.setup()) {
		return false;
	}
	if (!_shadowMapShader.setup()) {
		return false;
	}
	if (!_shadowMapRenderShader.setup()) {
		return false;
	}

	_rootNode = new RenderOctreeNode(_worldShader);
	_volume = new voxel::OctreeVolume(volume, region, baseNodeSize);
	_colorTexture.init();

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	video::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _shadowMapRenderShader.getLocationPos();
	attributePos.size = _shadowMapRenderShader.getComponentsPos();
	_shadowMapDebugBuffer.addAttribute(attributePos);

	video::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _shadowMapRenderShader.getLocationTexcoord();
	attributeTexcoord.size = _shadowMapRenderShader.getComponentsTexcoord();
	_shadowMapDebugBuffer.addAttribute(attributeTexcoord);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, maxDepthBuffers)) {
		return false;
	}

	const int shaderMaterialColorsArraySize = lengthof(shader::Materialblock::Data::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::Materialblock::Data materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);

	if (!_shadow.init()) {
		return false;
	}

	return true;
}

int OctreeRenderer::update(long dt, const video::Camera& camera) {
	if (_volume == nullptr) {
		return 0;
	}
	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	_shadow.calculateShadowData(camera, true, maxDepthBuffers, _depthBuffer.dimension());
	return _volume->update(dt, camera.position(), 1.0f);
}

void OctreeRenderer::shutdown() {
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
	_shadowMapInstancedShader.shutdown();
	_shadow.shutdown();
	_worldShader.shutdown();
	_worldInstancedShader.shutdown();
	_waterShader.shutdown();
	_shadowMapShader.shutdown();
	_depthBuffer.shutdown();
	_materialBlock.shutdown();
	_colorTexture.shutdown();
	delete _rootNode;
	_rootNode = nullptr;
	delete _volume;
	_volume = nullptr;
}

}
