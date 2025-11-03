/**
 * @file
 */

#include "VoxelModificationHandler.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

VoxelModificationHandler::VoxelModificationHandler(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

void VoxelModificationHandler::execute(const ClientId &, VoxelModificationMessage *message) {
	const core::UUID &uuid = message->nodeUUID();
	const core::String &uuidStr = uuid.str();
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(uuid);
	if (node == nullptr) {
		Log::warn("Received voxel modification for unknown node UUID %s", uuidStr.c_str());
		return;
	}
	if (!node->isModelNode()) {
		Log::warn("Received voxel modification for non-model node UUID %s", uuidStr.c_str());
		return;
	}
	const voxel::Region &region = message->region();
	if (!region.isValid()) {
		Log::warn("Received voxel modification with invalid region for node UUID %s", uuidStr.c_str());
		return;
	}
	const uint8_t *data = message->compressedData();
	const uint32_t dataSize = message->compressedSize();
	const size_t uncompressedBufferSize = message->region().voxels() * sizeof(voxel::Voxel);
	io::MemoryReadStream dataStream(data, dataSize);
	io::ZipReadStream stream(dataStream, (int)dataStream.size());
	uint8_t *uncompressedBuf = (uint8_t *)core_malloc(uncompressedBufferSize);
	if (stream.read(uncompressedBuf, uncompressedBufferSize) == -1) {
		core_free(uncompressedBuf);
		return;
	}
	core::ScopedPtr<voxel::RawVolume> v(
		voxel::RawVolume::createRaw((voxel::Voxel *)uncompressedBuf, message->region()));
	Client &client = _sceneMgr->client();
	client.lockListener();
	_sceneMgr->nodeUpdatePartialVolume(*node, *v);
	client.unlockListener();
}

} // namespace voxedit
