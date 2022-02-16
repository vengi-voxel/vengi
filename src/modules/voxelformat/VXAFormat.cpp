/**
 * @file
 */

#include "VXAFormat.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "app/App.h"
#include "VXMFormat.h"
#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load vxa file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vxa file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool VXAFormat::recursiveImportNode(const core::String &filename, io::SeekableReadStream &stream,
									SceneGraph &sceneGraph, SceneGraphNode& node, const core::String &animId) {
	int32_t keyFrameCount;
	wrap(stream.readInt32(keyFrameCount))
	Log::error("Found %i keyframes", keyFrameCount);
	for (int32_t i = 0u; i < keyFrameCount; ++i) {
		uint32_t frame;
		wrap(stream.readUInt32(frame))
		int32_t interpolation;
		wrap(stream.readInt32(interpolation))
		stream.readBool(); // rotation ??
		SceneGraphTransform transform;
		transform.normalizedPivot = glm::vec3(0.5f);
		glm::vec3 localPosition{0.0f};
		glm::quat localRot{0.0f, 0.0f, 0.0f, 0.0f};
		float localScale = 1.0f;
		wrap(stream.readFloat(transform.position.x))
		wrap(stream.readFloat(transform.position.y))
		wrap(stream.readFloat(transform.position.z))
		wrap(stream.readFloat(localPosition.x))
		wrap(stream.readFloat(localPosition.y))
		wrap(stream.readFloat(localPosition.z))
		wrap(stream.readFloat(transform.rot.x))
		wrap(stream.readFloat(transform.rot.y))
		wrap(stream.readFloat(transform.rot.z))
		wrap(stream.readFloat(transform.rot.w))
		wrap(stream.readFloat(localRot.x))
		wrap(stream.readFloat(localRot.y))
		wrap(stream.readFloat(localRot.z))
		wrap(stream.readFloat(localRot.w))
		wrap(stream.readFloat(transform.scale))
		wrap(stream.readFloat(localScale))
		// TODO: only the first frame is supported - as we don't have animation support yet
		if (i == 0u) {
			Log::error("Set transform for node %i", node.id());
			node.setTransform(transform, true);
		}
	}
	int32_t children;
	wrap(stream.readInt32(children))
	if (children != (int32_t)node.children().size()) {
		Log::error("Child count mismatch between loaded node %i and the vxa (%i)", node.id(), children);
		return false;
	}
	for (int32_t i = 0; i < children; ++i) {
		const int nodeId = node.children()[i];
		SceneGraphNode& cnode = sceneGraph.node(nodeId);
		wrapBool(recursiveImportNode(filename, stream, sceneGraph, cnode, animId))
	}

	return true;
}

bool VXAFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	uint8_t magic[4];
	wrap(stream.readUInt8(magic[0]))
	wrap(stream.readUInt8(magic[1]))
	wrap(stream.readUInt8(magic[2]))
	wrap(stream.readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'A') {
		Log::error("Could not load vxr file: Invalid magic found (%c%c%c%c)",
			magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version;
	if (magic[3] >= '0' && magic[3] <= '9') {
		version = magic[3] - '0';
	} else {
		Log::error("Invalid version found");
		return false;
	}

	Log::debug("Found vxa version: %i", version);

	if (version != 2) {
		Log::error("Could not load vxa file: Unsupported version found (%i)", version);
		return false;
	}

	if (sceneGraph.empty()) {
		Log::error("No previously loaded vma/vxr nodes found");
		return false;
	}

	int64_t md5[2];
	wrap(stream.readInt64(md5[0]))
	wrap(stream.readInt64(md5[1]))

	char animId[1024];
	wrapBool(stream.readString(sizeof(animId), animId, true))
	Log::debug("anim: '%s'", animId);
	int32_t rootChilds = 0;
	wrap(stream.readInt32(rootChilds))
	Log::debug("rootChilds: %i", rootChilds);
	if (rootChilds == 0) {
		Log::debug("No children node found in vxa - positioning might be wrong");
		return true;
	}

	const int32_t sceneGraphRootChilds = (int32_t)sceneGraph.root().children().size();
	if (rootChilds != sceneGraphRootChilds) {
		Log::error("VXA root child count doesn't match with current loaded scene graph %i vs %i", rootChilds, sceneGraphRootChilds);
		return false;
	}

	for (int32_t i = 0; i < rootChilds; ++i) {
		const int nodeId = sceneGraph.root().children()[i];
		SceneGraphNode& node = sceneGraph.node(nodeId);
		if (!recursiveImportNode(filename, stream, sceneGraph, node, animId)) {
			Log::error("VXA: failed to import children");
			return false;
		}
	}
	return true;
}

#undef wrap
#undef wrapBool

}
