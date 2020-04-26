/**
 * @file
 */

#include "WorldChunkMgr.h"
#include "core/Trace.h"
#include "video/Trace.h"
#include "voxel/Constants.h"
#include "voxelrender/ShaderAttribute.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace voxelrender {

WorldChunkMgr::WorldChunkMgr(core::ThreadPool& threadPool) :
		_octree({}, 30), _threadPool(threadPool) {
}

void WorldChunkMgr::updateViewDistance(float viewDistance) {
	const glm::vec3 cullingThreshold(_meshExtractor.meshSize());
	const int maxCullingThreshold = core_max(cullingThreshold.x, cullingThreshold.z) * 4;
	_maxAllowedDistance = glm::pow(viewDistance + maxCullingThreshold, 2);
}

bool WorldChunkMgr::init(shader::WorldShader* worldShader, voxel::PagedVolume* volume) {
	_worldShader = worldShader;
	if (!_meshExtractor.init(volume)) {
		Log::error("Failed to initialize the mesh extractor");
		return false;
	}
	return true;
}

void WorldChunkMgr::shutdown() {
	_meshExtractor.shutdown();
}

void WorldChunkMgr::reset() {
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		chunkBuffer.inuse = false;
	}
	_visibleBuffers.size = 0;
	_meshExtractor.reset();
	_octree.clear();
}

bool WorldChunkMgr::initTerrainBuffer(ChunkBuffer* chunk) {
	chunk->_vbo = chunk->_buffer.create();
	if (chunk->_vbo == -1) {
		Log::error("Failed to create vertex buffer");
		return false;
	}
	chunk->_ibo = chunk->_buffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (chunk->_ibo == -1) {
		Log::error("Failed to create index buffer");
		return false;
	}

	const int locationPos = _worldShader->getLocationPos();
	const video::Attribute& posAttrib = getPositionVertexAttribute(chunk->_vbo, locationPos, _worldShader->getAttributeComponents(locationPos));
	if (!chunk->_buffer.addAttribute(posAttrib)) {
		Log::warn("Failed to add position attribute");
	}

	const int locationInfo = _worldShader->getLocationInfo();
	const video::Attribute& infoAttrib = getInfoVertexAttribute(chunk->_vbo, locationInfo, _worldShader->getAttributeComponents(locationInfo));
	if (!chunk->_buffer.addAttribute(infoAttrib)) {
		Log::warn("Failed to add info attribute");
	}

	chunk->_buffer.update(chunk->_vbo, chunk->mesh.getVertexVector());
	chunk->_buffer.update(chunk->_ibo, chunk->mesh.getIndexVector());

	return true;
}

int WorldChunkMgr::renderTerrain(float nowSeconds) {
	video_trace_scoped(WorldChunkMgrRenderTerrain);
	int drawCalls = 0;

	for (int i = 0; i < _visibleBuffers.size; ++i) {
		ChunkBuffer& chunkBuffer = *_visibleBuffers.visible[i];
		core_assert(chunkBuffer.inuse);
		const video::Buffer& buffer = chunkBuffer._buffer;
		const int ibo = chunkBuffer._ibo;
		const uint32_t numIndices = buffer.elements(ibo, 1, sizeof(voxel::IndexType));
		if (numIndices == 0u) {
			return false;
		}
		video::ScopedBuffer scopedBuf(buffer);
		if (_worldShader->isActive()) {
			const float delta = glm::clamp((nowSeconds - chunkBuffer.birthSeconds) / 3.0f, 0.0f, 1.0f);
			const glm::vec3 size = glm::mix(glm::vec3(1.0f, 0.4f, 1.0f), glm::vec3(1.0f), delta);
			const glm::mat4& model = glm::scale(size);
			_worldShader->setModel(model);
		}
		video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
		++drawCalls;
	}
	return drawCalls;
}

void WorldChunkMgr::handleMeshQueue(float nowSeconds) {
	voxel::Mesh mesh;
	if (!_meshExtractor.pop(mesh)) {
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
		if (chunkBuffer.translation() == mesh.getOffset()) {
			freeChunkBuffer = &chunkBuffer;
			break;
		}
	}

	if (freeChunkBuffer == nullptr) {
		Log::warn("Could not find free chunk buffer slot");
		return;
	}

	freeChunkBuffer->mesh = std::move(mesh);
	freeChunkBuffer->_aabb = {freeChunkBuffer->mesh.mins(), freeChunkBuffer->mesh.maxs()};
	if (!_octree.insert(freeChunkBuffer)) {
		Log::warn("Failed to insert into octree");
	}
	freeChunkBuffer->inuse = true;
	freeChunkBuffer->birthSeconds = nowSeconds;
}

void WorldChunkMgr::update(float nowSeconds, const video::Camera &camera, const glm::vec3& focusPos) {
	handleMeshQueue(nowSeconds);

	_meshExtractor.updateExtractionOrder(focusPos);
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (!chunkBuffer.inuse) {
			continue;
		}
		if (chunkBuffer._ibo == -1) {
			initTerrainBuffer(&chunkBuffer);
		}
		const int distance = distance2(chunkBuffer.translation(), focusPos);
		if (distance < _maxAllowedDistance) {
			continue;
		}
		core_assert_always(_meshExtractor.allowReExtraction(chunkBuffer.translation()));
		chunkBuffer.reset();
		_octree.remove(&chunkBuffer);
		Log::trace("Remove mesh from %i:%i", chunkBuffer.translation().x, chunkBuffer.translation().z);
	}

	cull(camera);
}

void WorldChunkMgr::extractScheduledMesh() {
	_meshExtractor.extractScheduledMesh();
}

// TODO: put into background task with two states - computing and
// next - then the indices and vertices are just swapped
void WorldChunkMgr::cull(const video::Camera& camera) {
	core_trace_scoped(WorldRendererCull);

	math::AABB<float> aabb = camera.frustum().aabb();
	// don't cull objects that might cast shadows
	aabb.shift(camera.forward() * -10.0f);

	size_t index = 0;
	Tree::Contents contents;
	_octree.query(math::AABB<int>(aabb.mins(), aabb.maxs()), contents);

	for (ChunkBuffer* chunkBuffer : contents) {
		_visibleBuffers.visible[index++] = chunkBuffer;
	}
	_visibleBuffers.size = index;
}

int WorldChunkMgr::distance2(const glm::ivec3& pos, const glm::ivec3& pos2) const {
	// we are only taking the x and z axis into account here
	const glm::ivec2 dist(pos.x - pos2.x, pos.z - pos2.z);
	const int distance = dist.x * dist.x + dist.y * dist.y;
	return distance;
}

void WorldChunkMgr::extractMeshes(const video::Camera& camera) {
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
		return !_meshExtractor.scheduleMeshExtraction(mins);
	}, glm::vec3(_meshExtractor.meshSize()));
}

void WorldChunkMgr::extractMesh(const glm::ivec3& pos) {
	_meshExtractor.scheduleMeshExtraction(pos);
}

}