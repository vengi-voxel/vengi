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

namespace voxelworldrender {

namespace {
constexpr double ScaleDuration = 1.5;
}

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

void WorldChunkMgr::handleMeshQueue() {
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

	video::Buffer& buffer = freeChunkBuffer->_buffer;
	freeChunkBuffer->_vbo = buffer.create();
	if (freeChunkBuffer->_vbo == -1) {
		Log::error("Failed to create vertex buffer");
		return;
	}
	const int locationPos = _worldShader->getLocationPos();
	const video::Attribute& posAttrib = voxelrender::getPositionVertexAttribute(freeChunkBuffer->_vbo, locationPos, _worldShader->getAttributeComponents(locationPos));
	if (!buffer.addAttribute(posAttrib)) {
		Log::error("Failed to add position attribute");
		return;
	}
	const int locationInfo = _worldShader->getLocationInfo();
	const video::Attribute& infoAttrib = voxelrender::getInfoVertexAttribute(freeChunkBuffer->_vbo, locationInfo, _worldShader->getAttributeComponents(locationInfo));
	if (!buffer.addAttribute(infoAttrib)) {
		Log::error("Failed to add info attribute");
		return;
	}
	freeChunkBuffer->_ibo = buffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (freeChunkBuffer->_ibo == -1) {
		Log::error("Failed to create index buffer");
		return;
	}
	freeChunkBuffer->_offset = mesh.getOffset();
	freeChunkBuffer->_compressedIndexSize = mesh.compressedIndexSize();

	const voxel::VertexArray& vertices = mesh.getVertexVector();
	if (vertices.empty()) {
		buffer.update(freeChunkBuffer->_vbo, nullptr, 0);
		buffer.update(freeChunkBuffer->_ibo, nullptr, 0);
	} else {
		const uint8_t* indices = mesh.compressedIndices();
		buffer.update(freeChunkBuffer->_vbo, &vertices.front(), vertices.size() * sizeof(voxel::VertexArray::value_type));
		buffer.update(freeChunkBuffer->_ibo, indices, mesh.getNoOfIndices() * freeChunkBuffer->_compressedIndexSize);
	}

	const glm::ivec3& size = _meshExtractor.meshSize();
	const glm::ivec3& mins = mesh.getOffset();
	const glm::ivec3 maxs(mins.x + size.x, mins.y + size.y, mins.z + size.z);
	freeChunkBuffer->_aabb = {mins, maxs};
	if (!_octree.insert(freeChunkBuffer)) {
		Log::warn("Failed to insert into octree");
	}
	freeChunkBuffer->inuse = true;
	freeChunkBuffer->scaleSeconds = ScaleDuration;
}

void WorldChunkMgr::update(double deltaFrameSeconds, const video::Camera &camera, const glm::vec3& focusPos) {
	handleMeshQueue();

	_meshExtractor.updateExtractionOrder(focusPos);
	for (ChunkBuffer& chunkBuffer : _chunkBuffers) {
		if (!chunkBuffer.inuse) {
			continue;
		}
		chunkBuffer.scaleSeconds -= deltaFrameSeconds;
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

int WorldChunkMgr::renderTerrain() {
	video_trace_scoped(WorldChunkMgrRenderTerrain);
	int drawCalls = 0;

	for (int i = 0; i < _visibleBuffers.size; ++i) {
		ChunkBuffer& chunkBuffer = *_visibleBuffers.visible[i];
		core_assert(chunkBuffer.inuse);
		const video::Buffer& buffer = chunkBuffer._buffer;
		const int ibo = chunkBuffer._ibo;
		const uint32_t numIndices = buffer.elements(ibo, 1, chunkBuffer._compressedIndexSize);
		if (numIndices == 0u) {
			return false;
		}
		video::ScopedBuffer scopedBuf(buffer);
		if (_worldShader->isActive()) {
			const double delta = glm::clamp(core_max(0.0, chunkBuffer.scaleSeconds) / ScaleDuration, 0.0, 1.0);
			const glm::vec3 &size = glm::mix(glm::vec3(1.0f), glm::vec3(1.0f, 0.4f, 1.0f), (float)delta);
			const glm::mat4& model = glm::scale(size);
			_worldShader->setModel(model);
		}
		video::drawElements(video::Primitive::Triangles, numIndices, chunkBuffer._compressedIndexSize);
		++drawCalls;
	}
	return drawCalls;
}

}