#include "RawVolumeRenderer.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedPolygonMode.h"
#include "frontend/ShaderAttribute.h"

namespace frontend {

/// implementation of a function object for deciding when
/// the cubic surface extractor should insert a face between two voxels.
///
/// The criteria used here are that the voxel in front of the potential
/// quad should have a value of zero (which would typically indicate empty
/// space) while the voxel behind the potential quad would have a value
/// geater than zero (typically indicating it is solid).
struct CustomIsQuadNeeded {
	inline bool operator()(const voxel::VoxelType& back, const voxel::VoxelType& front, voxel::FaceNames face) const {
		if (isBlocked(back) && !isBlocked(front)) {
			return true;
		}
		return false;
	}
};

const std::string MaxDepthBufferUniformName = "u_cascades";

RawVolumeRenderer::RawVolumeRenderer() :
		_shadowMapShader(shader::ShadowmapShader::getInstance()),
		_worldShader(shader::WorldShader::getInstance()) {
	_sunDirection = glm::vec3(glm::left.x, glm::down.y, 0.0f);
}

bool RawVolumeRenderer::init() {
	if (!_worldShader.setup()) {
		Log::error("Failed to initialize the color shader");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to initialize the color shader");
		return false;
	}

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		_vertexBufferIndex[idx] = _vertexBuffer[idx].create();
		if (_vertexBufferIndex[idx] == -1) {
			Log::error("Could not create the vertex buffer object");
			return false;
		}

		_indexBufferIndex[idx] = _vertexBuffer[idx].create(nullptr, 0, video::VertexBufferType::IndexBuffer);
		if (_indexBufferIndex[idx] == -1) {
			Log::error("Could not create the vertex buffer object for the indices");
			return false;
		}
	}

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, video::DepthBufferMode::DEPTH_CMP, maxDepthBuffers)) {
		return false;
	}

	const int shaderMaterialColorsArraySize = SDL_arraysize(shader::Materialblock::Data::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::Materialblock::Data materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);
	video::ScopedShader scoped(_worldShader);
	_worldShader.setMaterialblock(_materialBlock);
	_worldShader.setModel(glm::mat4());
	_worldShader.setTexture(video::TextureUnit::Zero);
	_worldShader.setShadowmap(video::TextureUnit::One);
	_worldShader.setFogrange(250.0f);
	_worldShader.setDiffuseColor(_diffuseColor);
	_worldShader.setAmbientColor(_ambientColor);
	_worldShader.setFogcolor(core::Color::LightBlue);

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		video::Attribute attributePos = getPositionVertexAttribute(
				_vertexBufferIndex[idx], _worldShader.getLocationPos(),
				_worldShader.getComponentsPos());
		_vertexBuffer[idx].addAttribute(attributePos);

		video::Attribute attributeInfo = getInfoVertexAttribute(
				_vertexBufferIndex[idx], _worldShader.getLocationInfo(),
				_worldShader.getComponentsInfo());
		_vertexBuffer[idx].addAttribute(attributeInfo);
	}

	if (!_shadow.init()) {
		return false;
	}

	_whiteTexture = video::createWhiteTexture("**whitetexture**");

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		_mesh[idx] = new voxel::Mesh(128, 128, true);
	}

	return true;
}

bool RawVolumeRenderer::onResize(const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(RawVolumeRendererOnResize);
	return true;
}

bool RawVolumeRenderer::update(int idx, const std::vector<voxel::VoxelVertex>& vertices, const std::vector<voxel::IndexType>& indices) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	core_trace_scoped(RawVolumeRendererUpdate);
	if (!_vertexBuffer[idx].update(_vertexBufferIndex[idx], vertices)) {
		Log::error("Failed to update the vertex buffer");
		return false;
	}
	if (!_vertexBuffer[idx].update(_indexBufferIndex[idx], indices)) {
		Log::error("Failed to update the index buffer");
		return false;
	}
	return true;
}

void RawVolumeRenderer::extractAll() {
	core_trace_scoped(RawVolumeRendererExtract);
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		extract(idx);
	}
}

bool RawVolumeRenderer::extract(int idx) {
	if (_rawVolume[idx] == nullptr) {
		return false;
	}

	if (_mesh[idx] == nullptr) {
		return false;
	}
	voxel::Region r = _rawVolume[idx]->getRegion();
	r.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(_rawVolume[idx], r, _mesh[idx], CustomIsQuadNeeded());
	const size_t meshNumberIndices = _mesh[idx]->getNoOfIndices();
	if (meshNumberIndices == 0) {
		_vertexBuffer[idx].update(_vertexBufferIndex[idx], nullptr, 0);
		_vertexBuffer[idx].update(_indexBufferIndex[idx], nullptr, 0);
		return true;
	}
	update(idx, _mesh[idx]->getVertexVector(), _mesh[idx]->getIndexVector());
	return true;
}

void RawVolumeRenderer::render(const video::Camera& camera) {
	core_trace_scoped(RawVolumeRendererRender);

	uint32_t numIndices = 0u;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		numIndices += _vertexBuffer[idx].elements(_indexBufferIndex[idx], 1, sizeof(voxel::IndexType));
	}
	if (numIndices == 0u) {
		return;
	}

	const bool oldDepth = video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	const bool oldCullFace = video::enable(video::State::CullFace);
	const bool oldDepthMask = video::enable(video::State::DepthMask);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	_shadow.calculateShadowData(camera, true, maxDepthBuffers, _depthBuffer.dimension());
	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();
	{
		const bool oldBlend = video::disable(video::State::Blend);
		// put shadow acne into the dark
		video::cullFace(video::Face::Front);
		const float shadowBiasSlope = 2;
		const float shadowBias = 0.09f;
		const float shadowRangeZ = camera.farPlane() * 3.0f;
		const glm::vec2 offset(shadowBiasSlope, (shadowBias / shadowRangeZ) * (1 << 24));
		const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

		_depthBuffer.bind();
		video::ScopedShader scoped(_shadowMapShader);
		for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
			const uint32_t nIndices = _vertexBuffer[idx].elements(_indexBufferIndex[idx], 1, sizeof(voxel::IndexType));
			if (nIndices == 0) {
				continue;
			}
			core_assert_always(_vertexBuffer[idx].bind());
			_shadowMapShader.setModel(glm::translate(_offsets[idx]));
			for (int i = 0; i < maxDepthBuffers; ++i) {
				_depthBuffer.bindTexture(i);
				_shadowMapShader.setLightviewprojection(cascades[i]);
				static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
				video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
			}
			_vertexBuffer[idx].unbind();
		}
		video::cullFace(video::Face::Back);
		if (oldBlend) {
			video::enable(video::State::Blend);
		}
		_depthBuffer.unbind();
	}

	_whiteTexture->bind(video::TextureUnit::Zero);

	{
		video::ScopedShader scoped(_worldShader);
		_worldShader.setViewprojection(camera.viewProjectionMatrix());
		_worldShader.setViewdistance(camera.farPlane());
		_worldShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
		_worldShader.setCascades(cascades);
		_worldShader.setDistances(distances);
		_worldShader.setLightdir(_shadow.sunDirection());

		video::ScopedPolygonMode polygonMode(camera.polygonMode());
		video::bindTexture(video::TextureUnit::One, _depthBuffer);
		for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
			const uint32_t nIndices = _vertexBuffer[idx].elements(_indexBufferIndex[idx], 1, sizeof(voxel::IndexType));
			if (nIndices == 0) {
				continue;
			}
			core_assert_always(_vertexBuffer[idx].bind());
			_worldShader.setModel(glm::translate(_offsets[idx]));
			static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
			video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
			_vertexBuffer[idx].unbind();
		}
	}
	_whiteTexture->unbind();

	if (!oldDepth) {
		video::disable(video::State::DepthTest);
	}
	if (!oldCullFace) {
		video::disable(video::State::CullFace);
	}
	if (!oldDepthMask) {
		video::disable(video::State::DepthMask);
	}
}

bool RawVolumeRenderer::setOffset(int idx, const glm::ivec3& offset) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}

	_offsets[idx] = offset;

	return true;
}

voxel::RawVolume* RawVolumeRenderer::setVolume(int idx, voxel::RawVolume* volume, const glm::ivec3& offset) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	core_trace_scoped(RawVolumeRendererSetVolume);

	voxel::RawVolume* old = _rawVolume[idx];
	_rawVolume[idx] = volume;
	_offsets[idx] = offset;

	return old;
}

std::vector<voxel::RawVolume*> RawVolumeRenderer::shutdown() {
	_worldShader.shutdown();
	_shadowMapShader.shutdown();
	_materialBlock.shutdown();
	std::vector<voxel::RawVolume*> old(MAX_VOLUMES);
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		_vertexBuffer[idx].shutdown();
		_vertexBufferIndex[idx] = -1;
		_indexBufferIndex[idx] = -1;
		delete _mesh[idx];
		_mesh[idx] = nullptr;
		old.push_back(_rawVolume[idx]);
		_rawVolume[idx] = nullptr;
	}
	if (_whiteTexture) {
		_whiteTexture->shutdown();
		_whiteTexture = video::TexturePtr();
	}
	_depthBuffer.shutdown();
	return old;
}

}
