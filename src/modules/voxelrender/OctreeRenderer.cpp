/**
 * @file
 */

#include "OctreeRenderer.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedPolygonMode.h"
#include "core/Trace.h"
#include "core/Array.h"

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

	_shadow.render([this, camera] (int i, shader::ShadowmapShader& shader) {
		shader.setModel(glm::mat4(1.0f));
		renderOctreeNode(camera, _rootNode);
		return true;
	}, [] (int, shader::ShadowmapInstancedShader&) {
		return true;
	});
	video::cullFace(video::Face::Back);
	video::enable(video::State::Blend);
	_colorTexture.bind(video::TextureUnit::Zero);
	video::clearColor(_clearColor);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);
	_shadow.bind(video::TextureUnit::One);
	video::ScopedShader scoped(_worldShader);
	_worldShader.setMaterialblock(_materialBlock);
	_worldShader.setViewdistance(camera.farPlane());
	_worldShader.setLightdir(_shadow.sunDirection());
	_worldShader.setFogcolor(_clearColor);
	_worldShader.setTexture(video::TextureUnit::Zero);
	_worldShader.setDiffuseColor(_diffuseColor);
	_worldShader.setAmbientColor(_ambientColor);
	_worldShader.setFogrange(_fogRange);
	_worldShader.setModel(glm::mat4(1.0f));
	_worldShader.setViewprojection(camera.viewProjectionMatrix());
	_worldShader.setShadowmap(video::TextureUnit::One);
	_worldShader.setDepthsize(glm::vec2(_shadow.dimension()));
	_worldShader.setCascades(_shadow.cascades());
	_worldShader.setDistances(_shadow.distances());
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
	if (!_waterShader.setup()) {
		return false;
	}

	_rootNode = new RenderOctreeNode(_worldShader);
	_volume = new voxel::OctreeVolume(volume, region, baseNodeSize);
	_colorTexture.init();

	const int maxDepthBuffers = _worldShader.getUniformArraySize(shader::WorldShader::getMaxDepthBufferUniformName());
	if (!_shadow.init(maxDepthBuffers)) {
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

	return true;
}

int OctreeRenderer::update(long dt, const video::Camera& camera) {
	if (_volume == nullptr) {
		return 0;
	}
	_shadow.update(camera, true);
	return _volume->update(dt, camera.position(), 1.0f);
}

void OctreeRenderer::shutdown() {
	_shadow.shutdown();
	_worldShader.shutdown();
	_worldInstancedShader.shutdown();
	_waterShader.shutdown();
	_materialBlock.shutdown();
	_colorTexture.shutdown();
	delete _rootNode;
	_rootNode = nullptr;
	delete _volume;
	_volume = nullptr;
}

}
