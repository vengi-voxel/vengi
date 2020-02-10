/**
 * @file
 */

#include "WorldRenderer.h"
#include "ShaderAttribute.h"
#include "WorldShaderConstants.h"

#include "core/Color.h"
#include "core/GameConfig.h"
#include "core/ArrayLength.h"
#include "core/App.h"
#include "core/Var.h"
#include "core/GLM.h"
#include "voxel/Constants.h"
#include "voxel/MaterialColor.h"

#include "video/Renderer.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "video/Types.h"

namespace voxelrender {

// TODO: respect max vertex/index size of the one-big-vbo/ibo
WorldRenderer::WorldRenderer(const voxelworld::WorldMgrPtr& world) :
		_octree(math::AABB<float>(), 30), _world(world) {
	setViewDistance(240.0f);
}

WorldRenderer::~WorldRenderer() {
}

void WorldRenderer::reset() {
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		chunkBuffer.inuse = false;
	}
	_world->reset();
	_octree.clear();
	_activeChunkBuffers = 0;
	_entities.clear();
	_queryResults = 0;
	_now = 0ul;
}

void WorldRenderer::shutdown() {
	_worldShader.shutdown();
	_worldInstancedShader.shutdown();
	_waterShader.shutdown();
	_chrShader.shutdown();
	_materialBlock.shutdown();
	reset();
	_colorTexture.shutdown();
	_opaqueBuffer.shutdown();
	_waterBuffer.shutdown();
	_shadow.shutdown();
	_skybox.shutdown();
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_shapeRendererOcclusionQuery.shutdown();
	_shapeBuilderOcclusionQuery.shutdown();
	shutdownFrameBuffer();
	_postProcessBuf.shutdown();
	_postProcessBufId = -1;
	_postProcessShader.shutdown();

	for (int i = 0; i < MAX_CHUNKBUFFERS; ++i) {
		ChunkBuffer& buf = _chunkBuffers[i];
		video::deleteOcclusionQuery(buf.occlusionQueryId);
	}
}

frontend::ClientEntityPtr WorldRenderer::getEntity(frontend::ClientEntityId id) const {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		Log::warn("Could not get entity with id %li", id);
		return frontend::ClientEntityPtr();
	}
	return i->second;
}

bool WorldRenderer::addEntity(const frontend::ClientEntityPtr& entity) {
	auto i = _entities.find(entity->id());
	if (i != _entities.end()) {
		return false;
	}
	_entities[entity->id()] = entity;
	return true;
}

bool WorldRenderer::removeEntity(frontend::ClientEntityId id) {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		return false;
	}
	_entities.erase(i);
	return true;
}

// TODO: move into mesh extraction thread
void WorldRenderer::updateAABB(ChunkBuffer& chunkBuffer) const {
	core_trace_scoped(UpdateAABB);
	glm::ivec3 mins((std::numeric_limits<int>::max)());
	glm::ivec3 maxs((std::numeric_limits<int>::min)());

	const voxelworld::ChunkMeshes& meshes = chunkBuffer.meshes;
	for (auto& v : meshes.opaqueMesh.getVertexVector()) {
		mins = (glm::min)(mins, v.position);
		maxs = (glm::max)(maxs, v.position);
	}
	for (auto& v : meshes.waterMesh.getVertexVector()) {
		mins = (glm::min)(mins, v.position);
		maxs = (glm::max)(maxs, v.position);
	}

	chunkBuffer._aabb = {mins, maxs};
}

void WorldRenderer::handleMeshQueue() {
	voxelworld::ChunkMeshes meshes(0, 0, 0, 0);
	if (!_world->pop(meshes)) {
		return;
	}
	// Now add the mesh to the list of meshes to render.
	core_trace_scoped(WorldRendererHandleMeshQueue);

	ChunkBuffer* freeChunkBuffer = nullptr;
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (freeChunkBuffer == nullptr && !chunkBuffer.inuse) {
			freeChunkBuffer = &chunkBuffer;
		}
		// check whether we update an existing one
		if (chunkBuffer.translation() == meshes.translation()) {
			freeChunkBuffer = &chunkBuffer;
			break;
		}
	}

	if (freeChunkBuffer == nullptr) {
		Log::warn("Could not find free chunk buffer slot");
		return;
	}
	if (freeChunkBuffer->occlusionQueryId == video::InvalidId) {
		freeChunkBuffer->occlusionQueryId = video::genOcclusionQuery();
	}

	freeChunkBuffer->meshes = std::move(meshes);
	updateAABB(*freeChunkBuffer);
	if (!_octree.insert(freeChunkBuffer)) {
		Log::warn("Failed to insert into octree");
	}
	if (!freeChunkBuffer->inuse) {
		freeChunkBuffer->inuse = true;
		++_activeChunkBuffers;
	}
}

WorldRenderer::ChunkBuffer* WorldRenderer::findFreeChunkBuffer() {
	for (int i = 0; i < lengthof(_chunkBuffers); ++i) {
		if (!_chunkBuffers[i].inuse) {
			return &_chunkBuffers[i];
		}
	}
	return nullptr;
}

static inline size_t transform(size_t indexOffset, const voxel::Mesh& mesh, std::vector<voxel::VoxelVertex>& verts, std::vector<voxel::IndexType>& idxs) {
	const std::vector<voxel::IndexType>& indices = mesh.getIndexVector();
	const size_t start = idxs.size();
	idxs.insert(idxs.end(), indices.begin(), indices.end());
	const size_t end = idxs.size();
	for (size_t i = start; i < end; ++i) {
		idxs[i] += indexOffset;
	}
	const std::vector<voxel::VoxelVertex>& vertices = mesh.getVertexVector();
	verts.insert(verts.end(), vertices.begin(), vertices.end());
	return vertices.size();
}

bool WorldRenderer::occluded(ChunkBuffer * chunkBuffer) const {
	const video::Id queryId = chunkBuffer->occlusionQueryId;
	const int samples = video::getOcclusionQueryResult(queryId);
	if (samples == -1) {
		return chunkBuffer->occludedLastFrame;
	}
	chunkBuffer->occludedLastFrame = samples < _occlusionThreshold->intVal();
	chunkBuffer->pendingResult = false;
	return chunkBuffer->occludedLastFrame;
}

void WorldRenderer::cull(const video::Camera& camera) {
	core_trace_scoped(WorldRendererCull);
	_opaqueIndices.clear();
	_opaqueVertices.clear();
	_waterIndices.clear();
	_waterVertices.clear();
	size_t opaqueIndexOffset = 0;
	size_t waterIndexOffset = 0;
	_visibleChunks = 0;
	_occludedChunks = 0;

	const bool occlusionQuery = _occlusionQuery->boolVal();

	Tree::Contents contents;
	math::AABB<float> aabb = camera.frustum().aabb();
	aabb.shift(camera.forward() * -10.0f);
	_octree.query(aabb, contents);
	_queryResults = contents.size();

#if 0
	class VisibleSorter {
	private:
		const glm::vec3 _pos;
	public:
		VisibleSorter(const glm::vec3& pos) :
				_pos(pos) {
		}

		inline float dist(const glm::vec3& pos) const {
			return glm::distance2(_pos, pos);
		}

		inline bool operator()(const ChunkBuffer* lhs, const ChunkBuffer* rhs) const {
			const float lhsDist = dist(lhs->translation());
			const float rhsDist = dist(rhs->translation());
			return lhsDist < rhsDist;
		}
	};
	std::sort(contents.begin(), contents.end(), VisibleSorter(camera.position()));
#endif

	if (occlusionQuery) {
		core_trace_scoped(WorldRendererOcclusionQuery);
		// disable writing to the color buffer
		// We just want to check whether they would be rendered, not actually render them
		video::colorMask(false, false, false, false);

		for (ChunkBuffer* chunkBuffer : contents) {
			if (chunkBuffer->pendingResult) {
#if 0
				const math::AABB<int>& aabb = chunkBuffer->aabb();
				const glm::vec3& center = glm::vec3(aabb.getCenter());
				const glm::mat4& translate = glm::translate(center);
				const glm::mat4& model = glm::scale(translate, glm::vec3(aabb.getWidth()));
				_shapeRendererOcclusionQuery.render(_aabbMeshesOcclusionQuery, camera, model);
#endif
				continue;
			}
			const auto& aabb = chunkBuffer->aabb();
			if (aabb.containsPoint(camera.position())) {
				continue;
			}
			const video::Id queryId = chunkBuffer->occlusionQueryId;
			const glm::vec3& center = glm::vec3(aabb.getCenter());
			const glm::mat4& translate = glm::translate(center);
			const glm::mat4& model = glm::scale(translate, glm::vec3(aabb.getWidth()));
			core_assert(queryId != video::InvalidId);
			core_assert_always(video::beginOcclusionQuery(queryId));
			_shapeRendererOcclusionQuery.render(_aabbMeshesOcclusionQuery, camera, model);
			core_assert_always(video::endOcclusionQuery(queryId));
			chunkBuffer->pendingResult = true;
		}
		video::flush();
	}

	const bool renderOccluded = _renderOccluded->boolVal();
	const bool renderAABB = _renderAABBs->boolVal();
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Green);
	for (ChunkBuffer* chunkBuffer : contents) {
		core_trace_scoped(WorldRendererCullChunk);
		if (occlusionQuery && occluded(chunkBuffer)) {
			 ++_occludedChunks;
			 if (!renderOccluded) {
				 continue;
			 }
		} else if (renderOccluded) {
			++_visibleChunks;
			continue;
		} else {
			++_visibleChunks;
		}
		if (renderAABB) {
			_shapeBuilder.aabb(chunkBuffer->aabb());
		}
		const voxelworld::ChunkMeshes& meshes = chunkBuffer->meshes;
		opaqueIndexOffset += transform(opaqueIndexOffset, meshes.opaqueMesh, _opaqueVertices, _opaqueIndices);
		waterIndexOffset += transform(waterIndexOffset, meshes.waterMesh, _waterVertices, _waterIndices);
	}

	video::colorMask(true, true, true, true);
}

bool WorldRenderer::renderOpaqueBuffers() {
	const uint32_t numIndices = _opaqueBuffer.elements(_opaqueIbo, 1, sizeof(voxel::IndexType));
	if (numIndices == 0u) {
		return false;
	}
	video::ScopedBuffer scopedBuf(_opaqueBuffer);
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
	return true;
}

bool WorldRenderer::renderWaterBuffers() {
	const uint32_t numIndices = _waterBuffer.elements(_waterIbo, 1, sizeof(voxel::IndexType));
	if (numIndices == 0u) {
		return false;
	}
	video::ScopedState cullFace(video::State::CullFace, false);
	video::ScopedBuffer scopedBuf(_waterBuffer);
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
	return true;
}

int WorldRenderer::renderWorld(const video::Camera& camera, int* vertices) {
	core_trace_scoped(WorldRendererRenderWorld);
	handleMeshQueue();

	cull(camera);
	if (vertices != nullptr) {
		*vertices = _opaqueVertices.size() + _waterVertices.size();
	}
	if (_visibleChunks == 0) {
		return 0;
	}
	if (_opaqueIndices.empty() && _waterIndices.empty()) {
		return 0;
	}

	_frameBuffer.bind(true);
	const int drawCallsWorld = renderToFrameBuffer(camera);
	_frameBuffer.unbind();

	{
		video::ScopedState depthTest(video::State::DepthTest, false);
		const video::TexturePtr& fboTexture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
		video::ScopedShader scoped(_postProcessShader);
		video::ScopedTexture scopedTex(fboTexture, video::TextureUnit::Zero);
		video::ScopedBuffer scopedBuf(_postProcessBuf);
		const int currentEyeHeight = (int)camera.eye().y;
		if (currentEyeHeight < voxel::MAX_WATER_HEIGHT) {
			static const voxel::Voxel waterVoxel = voxel::createColorVoxel(voxel::VoxelType::Water, 0);
			const glm::vec4& waterColor = voxel::getMaterialColor(waterVoxel);
			_postProcessShader.setColor(waterColor);
		} else {
			_postProcessShader.setColor(glm::one<glm::vec4>());
		}
		_postProcessShader.setTexture(video::TextureUnit::Zero);
		const int elements = _postProcessBuf.elements(_postProcessBufId, _postProcessShader.getComponentsPos());
		video::drawArrays(video::Primitive::Triangles, elements);
	}

	// debug rendering
	const bool shadowMap = _shadowMap->boolVal();
	if (shadowMap && _shadowMapShow->boolVal()) {
		_shadow.renderShadowMap(camera);
	}

	if (_renderAABBs->boolVal()) {
		_shapeRenderer.createOrUpdate(_aabbMeshes, _shapeBuilder);
		_shapeRenderer.render(_aabbMeshes, camera);
	}

	return drawCallsWorld;
}

int WorldRenderer::renderToFrameBuffer(const video::Camera& camera) {
	core_assert_always(_opaqueBuffer.update(_opaqueVbo, _opaqueVertices));
	core_assert_always(_opaqueBuffer.update(_opaqueIbo, _opaqueIndices));
	core_assert_always(_waterBuffer.update(_waterVbo, _waterVertices));
	core_assert_always(_waterBuffer.update(_waterIbo, _waterIndices));

	int drawCallsWorld = 0;

	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	const bool shadowMap = _shadowMap->boolVal();
	if (shadowMap) {
		core_trace_scoped(WorldRendererRenderShadow);
		_shadow.render([&] (int i, shader::ShadowmapShader& shader) {
			shader.setModel(glm::mat4(1.0f));
			renderOpaqueBuffers();
			++drawCallsWorld;
			return true;
		}, [&] (int i, shader::ShadowmapInstancedShader& shader) {
			return true;
		});
	}
	_colorTexture.bind(video::TextureUnit::Zero);

	video::clearColor(_clearColor);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);

	if (shadowMap) {
		_shadow.bind(video::TextureUnit::One);
	}

	{
		core_trace_scoped(WorldRendererRenderOpaque);
		video::ScopedShader scoped(_worldShader);
		_worldShader.setModel(glm::mat4(1.0f));
		_worldShader.setMaterialblock(_materialBlock);
		_worldShader.setFocuspos(_focusPos);
		_worldShader.setLightdir(_shadow.sunDirection());
		_worldShader.setFogcolor(_clearColor);
		_worldShader.setTexture(video::TextureUnit::Zero);
		_worldShader.setDiffuseColor(_diffuseColor);
		_worldShader.setAmbientColor(_ambientColor);
		_worldShader.setNightColor(_nightColor);
		_worldShader.setTime(_seconds);
		_worldShader.setFogrange(_fogRange);
		if (shadowMap) {
			_worldShader.setViewprojection(camera.viewProjectionMatrix());
			_worldShader.setShadowmap(video::TextureUnit::One);
			_worldShader.setDepthsize(glm::vec2(_shadow.dimension()));
			_worldShader.setCascades(_shadow.cascades());
			_worldShader.setDistances(_shadow.distances());
		}
		if (renderOpaqueBuffers()) {
			++drawCallsWorld;
		}
	}
	drawCallsWorld += renderEntities(camera);
	{
		_skybox.bind(video::TextureUnit::Two);
		core_trace_scoped(WorldRendererRenderWater);
		video::ScopedShader scoped(_waterShader);
		_waterShader.setModel(glm::mat4(1.0f));
		_waterShader.setFocuspos(_focusPos);
		_waterShader.setCubemap(video::TextureUnit::Two);
		_waterShader.setCamerapos(camera.position());
		_waterShader.setLightdir(_shadow.sunDirection());
		_waterShader.setMaterialblock(_materialBlock);
		_waterShader.setFogcolor(_clearColor);
		_waterShader.setDiffuseColor(_diffuseColor);
		_waterShader.setAmbientColor(_ambientColor);
		_waterShader.setNightColor(_nightColor);
		_waterShader.setFogrange(_fogRange);
		_waterShader.setTime(_seconds);
		_waterShader.setTexture(video::TextureUnit::Zero);
		if (shadowMap) {
			_waterShader.setViewprojection(camera.viewProjectionMatrix());
			_waterShader.setShadowmap(video::TextureUnit::One);
			_waterShader.setDepthsize(glm::vec2(_shadow.dimension()));
			_waterShader.setCascades(_shadow.cascades());
			_waterShader.setDistances(_shadow.distances());
		}
		if (renderWaterBuffers()) {
			++drawCallsWorld;
		}
	}

	_skybox.render(camera);
	video::bindVertexArray(video::InvalidId);
	_colorTexture.unbind();

	return drawCallsWorld;
}

int WorldRenderer::renderEntities(const video::Camera& camera) {
	if (_entities.empty()) {
		return 0;
	}
	core_trace_gl_scoped(WorldRendererRenderEntities);

	int drawCallsEntities = 0;

	video::enable(video::State::DepthTest);
	video::enable(video::State::DepthMask);
	video::ScopedShader scoped(_chrShader);
	_chrShader.setFogrange(_fogRange);
	_chrShader.setFocuspos(_focusPos);
	_chrShader.setDiffuseColor(_diffuseColor);
	_chrShader.setAmbientColor(_ambientColor);
	_chrShader.setFogcolor(_clearColor);
	_chrShader.setCascades(_shadow.cascades());
	_chrShader.setDistances(_shadow.distances());
	_chrShader.setLightdir(_shadow.sunDirection());
	_chrShader.setNightColor(_nightColor);
	_chrShader.setTime(_seconds);

	const bool shadowMap = _shadowMap->boolVal();
	if (shadowMap) {
		_chrShader.setDepthsize(glm::vec2(_shadow.dimension()));
		_chrShader.setViewprojection(camera.viewProjectionMatrix());
		_chrShader.setShadowmap(video::TextureUnit::One);
		_shadow.bind(video::TextureUnit::One);
	}
	for (const auto& e : _entities) {
		const frontend::ClientEntityPtr& ent = e.second;
		ent->update(_deltaFrame);
		// note, that the aabb does not include the orientation - that should be kept in mind here.
		// a particular rotation could lead to an entity getting culled even though it should still
		// be visible.
		math::AABB<float> aabb = ent->character().aabb();
		aabb.shift(ent->position());
		if (!camera.isVisible(aabb)) {
			continue;
		}
		const glm::mat4& translate = glm::translate(ent->position());
		// as our models are looking along the positive z-axis, we have to rotate by 180 degree here
		const glm::mat4& model = glm::rotate(translate, glm::pi<float>() + ent->orientation(), glm::up);
		_chrShader.setModel(model);
		glm::mat4 bones[shader::SkeletonShaderConstants::getMaxBones()];
		const animation::Character& chr = ent->character();
		const animation::AnimationSettings& settings = chr.animationSettings();
		const animation::Skeleton& skeleton = chr.skeleton();
		skeleton.update(settings, bones);
		core_assert_always(_chrShader.setBones(bones));
		const uint32_t numIndices = ent->bindVertexBuffers(_chrShader);
		++drawCallsEntities;
		video::drawElements<animation::IndexType>(video::Primitive::Triangles, numIndices);
		ent->unbindVertexBuffers();
	}
	return drawCallsEntities;
}

void WorldRenderer::extractMesh(const glm::ivec3& pos) {
	_world->scheduleMeshExtraction(pos);
}

void WorldRenderer::extractMeshes(const video::Camera& camera) {
	core_trace_scoped(WorldRendererExtractMeshes);

	const float farplane = camera.farPlane();

	glm::vec3 mins = camera.position();
	mins.x -= farplane;
	mins.y = 0;
	mins.z -= farplane;

	glm::vec3 maxs = camera.position();
	maxs.x += farplane;
	maxs.y = voxel::MAX_HEIGHT;
	maxs.z += farplane;

	_octree.visit(mins, maxs, [&] (const glm::ivec3& mins, const glm::ivec3& maxs) {
		return !_world->scheduleMeshExtraction(mins);
	}, glm::vec3(_world->meshSize()));
}

void WorldRenderer::stats(Stats& stats) const {
	_world->stats(stats.meshes, stats.extracted, stats.pending);
	stats.active = _activeChunkBuffers;
	stats.visible = _visibleChunks;
	stats.occluded = _occludedChunks;
	stats.octreeSize = _octree.count();
	stats.octreeActive = _queryResults;
	core_assert(_visibleChunks == _queryResults - _occludedChunks);
}

void WorldRenderer::construct() {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_shadowMapShow = core::Var::get(cfg::ClientShadowMapShow, "false");
	_renderAABBs = core::Var::get(cfg::RenderAABB, "false");
	_occlusionThreshold = core::Var::get(cfg::OcclusionThreshold, "20");
	_occlusionQuery = core::Var::get(cfg::OcclusionQuery, "false");
	_renderOccluded = core::Var::get(cfg::RenderOccluded, "false");
}

bool WorldRenderer::initOpaqueBuffer() {
	_opaqueVbo = _opaqueBuffer.create();
	if (_opaqueVbo == -1) {
		Log::error("Failed to create vertex buffer");
		return false;
	}
	_opaqueBuffer.setMode(_opaqueVbo, video::BufferMode::Stream);
	_opaqueIbo = _opaqueBuffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_opaqueIbo == -1) {
		Log::error("Failed to create index buffer");
		return false;
	}
	_opaqueBuffer.setMode(_opaqueIbo, video::BufferMode::Stream);

	const int locationPos = _worldShader.getLocationPos();
	const video::Attribute& posAttrib = getPositionVertexAttribute(_opaqueVbo, locationPos, _worldShader.getAttributeComponents(locationPos));
	if (!_opaqueBuffer.addAttribute(posAttrib)) {
		Log::error("Failed to add position attribute");
		return false;
	}

	const int locationInfo = _worldShader.getLocationInfo();
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_opaqueVbo, locationInfo, _worldShader.getAttributeComponents(locationInfo));
	if (!_opaqueBuffer.addAttribute(infoAttrib)) {
		Log::error("Failed to add info attribute");
		return false;
	}

	return true;
}

bool WorldRenderer::initWaterBuffer() {
	_waterVbo = _waterBuffer.create();
	if (_waterVbo == -1) {
		Log::error("Failed to create water vertex buffer");
		return false;
	}
	_waterBuffer.setMode(_waterVbo, video::BufferMode::Stream);
	_waterIbo = _waterBuffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_waterIbo == -1) {
		Log::error("Failed to create water index buffer");
		return false;
	}
	_waterBuffer.setMode(_waterIbo, video::BufferMode::Stream);

	video::ScopedBuffer scoped(_waterBuffer);
	const int locationPos = _waterShader.getLocationPos();
	if (locationPos == -1) {
		Log::error("Failed to get pos location in water shader");
		return false;
	}
	_waterShader.enableVertexAttributeArray(locationPos);
	const video::Attribute& posAttrib = getPositionVertexAttribute(_waterVbo, locationPos, _waterShader.getAttributeComponents(locationPos));
	if (!_waterBuffer.addAttribute(posAttrib)) {
		Log::error("Failed to add water position attribute");
		return false;
	}

	const int locationInfo = _waterShader.getLocationInfo();
	if (locationInfo == -1) {
		Log::error("Failed to get info location in water shader");
		return false;
	}
	_waterShader.enableVertexAttributeArray(locationInfo);
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_waterVbo, locationInfo, _waterShader.getAttributeComponents(locationInfo));
	if (!_waterBuffer.addAttribute(infoAttrib)) {
		Log::error("Failed to add water info attribute");
		return false;
	}

	return true;
}

bool WorldRenderer::init(const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(WorldRendererOnInit);
	_colorTexture.init();

	if (!_shapeRenderer.init()) {
		Log::error("Failed to init the shape renderer");
		return false;
	}

	if (!_shapeRendererOcclusionQuery.init()) {
		Log::error("Failed to init the shape renderer");
		return false;
	}

	_shapeBuilderOcclusionQuery.setPosition(glm::vec3(0.0f));
	_shapeBuilderOcclusionQuery.setColor(core::Color::Red);
	_shapeBuilderOcclusionQuery.cube(glm::vec3(-0.5f), glm::vec3(0.5f));
	_aabbMeshesOcclusionQuery = _shapeRendererOcclusionQuery.create(_shapeBuilderOcclusionQuery);

	if (!_worldShader.setup()) {
		Log::error("Failed to setup the post world shader");
		return false;
	}
	if (!_worldInstancedShader.setup()) {
		Log::error("Failed to setup the post instancing shader");
		return false;
	}
	if (!_waterShader.setup()) {
		Log::error("Failed to setup the post water shader");
		return false;
	}
	if (!_chrShader.setup()) {
		Log::error("Failed to setup the post skeleton shader");
		return false;
	}
	if (!_postProcessShader.setup()) {
		Log::error("Failed to setup the post processing shader");
		return false;
	}
	if (!_skybox.init("sky")) {
		Log::error("Failed to initialize the sky");
		return false;
	}

	const int shaderMaterialColorsArraySize = lengthof(shader::WorldData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = (int)voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::WorldData::MaterialblockData materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);

	if (!initOpaqueBuffer()) {
		return false;
	}

	if (!initWaterBuffer()) {
		return false;
	}

	render::ShadowParameters sp;
	sp.shadowBias = -0.1f;
	sp.shadowBiasSlope = 0.1f;
	sp.sliceWeight = -4.0;
	sp.maxDepthBuffers = _worldShader.getUniformArraySize(shader::WorldShaderConstants::getMaxDepthBufferUniformName());
	if (!_shadow.init(sp)) {
		return false;
	}

	const glm::vec3 cullingThreshold(_world->meshSize());
	const int maxCullingThreshold = core_max(cullingThreshold.x, cullingThreshold.z) * 40;
	_maxAllowedDistance = glm::pow(_viewDistance + maxCullingThreshold, 2);

	initFrameBuffer(dimension);
	_postProcessBufId = _postProcessBuf.createFullscreenTextureBufferYFlipped();
	if (_postProcessBufId == -1) {
		return false;
	}

	struct VertexFormat {
		constexpr VertexFormat(const glm::vec2& p, const glm::vec2& t) : pos(p), tex(t) {}
		glm::vec2 pos;
		glm::vec2 tex;
	};
	alignas(16) constexpr VertexFormat vecs[] = {
		// left bottom
		VertexFormat(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f)),
		// right bottom
		VertexFormat(glm::vec2( 1.0f, -1.0f), glm::vec2(1.0f, 0.0f)),
		// right top
		VertexFormat(glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f)),
		// left bottom
		VertexFormat(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f)),
		// right top
		VertexFormat(glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f)),
		// left top
		VertexFormat(glm::vec2(-1.0f,  1.0f), glm::vec2(0.0f, 1.0f)),
	};

	_postProcessBufId = _postProcessBuf.create(vecs, sizeof(vecs));
	_postProcessBuf.addAttribute(_postProcessShader.getPosAttribute(_postProcessBufId, &VertexFormat::pos));
	_postProcessBuf.addAttribute(_postProcessShader.getTexcoordAttribute(_postProcessBufId, &VertexFormat::tex));

	return true;
}

void WorldRenderer::initFrameBuffer(const glm::ivec2& dimensions) {
	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	textureCfg.format(video::TextureFormat::RGBA);
	glm::vec2 frameBufferSize(dimensions.x, dimensions.y);
	video::FrameBufferConfig cfg;
	cfg.dimension(frameBufferSize).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24);
	cfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	_frameBuffer.init(cfg);
}

void WorldRenderer::shutdownFrameBuffer() {
	_frameBuffer.shutdown();
}

void WorldRenderer::update(const video::Camera& camera, uint64_t dt) {
	core_trace_scoped(WorldRendererOnRunning);
	_now += dt;
	_deltaFrame = dt;

	_focusPos = camera.target();
	_focusPos.y = _world->findFloor(_focusPos.x, _focusPos.z, voxel::isFloor);

	_world->updateExtractionOrder(_focusPos);

	const bool shadowMap = _shadowMap->boolVal();
	_shadow.update(camera, shadowMap);

	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (!chunkBuffer.inuse) {
			continue;
		}
		const int distance = getDistanceSquare(chunkBuffer.translation(), _focusPos);
		if (distance < _maxAllowedDistance) {
			continue;
		}
		core_assert_always(_world->allowReExtraction(chunkBuffer.translation()));
		chunkBuffer.inuse = false;
		--_activeChunkBuffers;
		_octree.remove(&chunkBuffer);
		video::deleteOcclusionQuery(chunkBuffer.occlusionQueryId);
		Log::trace("Remove mesh from %i:%i", chunkBuffer.translation().x, chunkBuffer.translation().z);
	}

	for (const auto& e : _entities) {
		e.second->update(dt);
	}
}

int WorldRenderer::getDistanceSquare(const glm::ivec3& pos, const glm::ivec3& pos2) const {
	const glm::ivec3 dist = pos - pos2;
	const int distance = dist.x * dist.x + dist.z * dist.z;
	return distance;
}

}
