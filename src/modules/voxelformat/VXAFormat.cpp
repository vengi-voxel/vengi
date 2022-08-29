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
#include "core/MD5.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

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

namespace vxa_priv {
static const InterpolationType interpolationTypes[]{
	InterpolationType::Instant,		 InterpolationType::Linear,			InterpolationType::QuadEaseIn,
	InterpolationType::QuadEaseOut,	 InterpolationType::QuadEaseInOut,	InterpolationType::CubicEaseIn,
	InterpolationType::CubicEaseOut, InterpolationType::CubicEaseInOut,
};

static void addNodeToHashStream_r(const SceneGraph& sceneGraph, const SceneGraphNode &node, io::WriteStream &stream) {
	stream.writeString(node.name(), false);
	const core::String &childHex = core::string::toHex((int32_t)node.children().size()).toUpper();
	stream.writeString(childHex, false);
	for (int child : node.children()) {
		addNodeToHashStream_r(sceneGraph, sceneGraph.node(child), stream);
	}
}

static void calculateHash(const SceneGraph& sceneGraph, uint64_t hash[2]) {
	io::BufferedReadWriteStream stream;
	const voxelformat::SceneGraphNode &root = sceneGraph.root();
	const voxelformat::SceneGraphNodeChildren &children = root.children();

	const int childCount = (int)children.size();
	if (childCount != 1 || sceneGraph.node(children[0]).name() != "Controller") {
		// add controller node (see VXRFormat)
		stream.writeString("Controller", false);
		stream.writeString(core::string::toHex(childCount).toUpper(), false);
	}
	for (int child : children) {
		const voxelformat::SceneGraphNode &node = sceneGraph.node(child);
		addNodeToHashStream_r(sceneGraph, node, stream);
	}
	uint8_t digest[16];
	core::md5sum(stream.getBuffer(), stream.size(), digest);
	// TODO: use MemoryReadStream and two times the readUInt64
	constexpr int half = lengthof(digest) / 2;
	hash[0] = hash[1] = 0;
	for (int i = 0; i < half; ++i) {
		hash[0] <<= 8;
		hash[0] |= digest[i];
	}
	for (int i = half; i < lengthof(digest); ++i) {
		hash[1] <<= 8;
		hash[1] |= digest[i];
	}
	Log::debug("hash: %s", core::md5ToString(digest).c_str());
}

static int getInterpolationType(InterpolationType type) {
	for (int i = 0; i < lengthof(interpolationTypes); ++i) {
		if (interpolationTypes[i] == type) {
			return i;
		}
	}
	return -1;
}

}

bool VXAFormat::recursiveImportNode(const core::String &filename, io::SeekableReadStream &stream,
									SceneGraph &sceneGraph, SceneGraphNode& node, const core::String &animId) {
	int32_t keyFrameCount;
	wrap(stream.readInt32(keyFrameCount))
	Log::debug("Found %i keyframes", keyFrameCount);
	for (int32_t i = 0u; i < keyFrameCount; ++i) {
		SceneGraphKeyFrame &keyFrame = node.keyFrame(i);
		uint32_t frame;
		wrap(stream.readUInt32(frame))
		keyFrame.frame = frame;
		int32_t interpolation;
		wrap(stream.readInt32(interpolation))
		if (interpolation < 0 || interpolation >= lengthof(vxa_priv::interpolationTypes)) {
			keyFrame.interpolation = InterpolationType::Linear;
			Log::warn("Could not find a supported easing type for %i", interpolation);
		} else {
			keyFrame.interpolation = vxa_priv::interpolationTypes[interpolation];
		}
		keyFrame.longRotation = stream.readBool();

		SceneGraphTransform &transform = keyFrame.transform();

		glm::vec3 localTranslation;
		glm::quat localOrientation;
		glm::vec3 translation;
		glm::quat orientation;
		float scale;
		float localScale;

		wrap(stream.readFloat(translation.x))
		wrap(stream.readFloat(translation.y))
		wrap(stream.readFloat(translation.z))
		wrap(stream.readFloat(localTranslation.x))
		wrap(stream.readFloat(localTranslation.y))
		wrap(stream.readFloat(localTranslation.z))
		wrap(stream.readFloat(orientation.x))
		wrap(stream.readFloat(orientation.y))
		wrap(stream.readFloat(orientation.z))
		wrap(stream.readFloat(orientation.w))
		wrap(stream.readFloat(localOrientation.x))
		wrap(stream.readFloat(localOrientation.y))
		wrap(stream.readFloat(localOrientation.z))
		wrap(stream.readFloat(localOrientation.w))
		wrap(stream.readFloat(scale))
		wrap(stream.readFloat(localScale))

		transform.setTranslation(translation);
		transform.setOrientation(orientation);
		transform.setScale(scale);
		transform.setLocalTranslation(localTranslation);
		transform.setLocalOrientation(localOrientation);
		transform.setLocalScale(localScale);
		transform.update();
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
		Log::error("Could not load vxa file: Invalid magic found (%c%c%c%c)",
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

	if (version > 2) {
		Log::error("Could not load vxa file: Unsupported version found (%i)", version);
		return false;
	}

	if (sceneGraph.empty()) {
		Log::error("No previously loaded vxm/vxr nodes found");
		return false;
	}

	uint64_t md5[2];
	wrap(stream.readUInt64(md5[0]))
	wrap(stream.readUInt64(md5[1]))

	uint64_t hash[2] {0};
	vxa_priv::calculateHash(sceneGraph, hash);

	if (SDL_memcmp(md5, hash, sizeof(hash))) {
		Log::error("hash checksums differ from vxa to current scene graph nodes (version: %i)", version);
		return false;
	}

	char animId[1024];
	wrapBool(stream.readString(sizeof(animId), animId, true))
	Log::debug("anim: '%s'", animId);
	int32_t rootChildren = 0;
	wrap(stream.readInt32(rootChildren))
	Log::debug("rootChildren: %i", rootChildren);
	if (rootChildren == 0) {
		Log::debug("No children node found in vxa - positioning might be wrong");
		return true;
	}

	const int32_t sceneGraphRootChildren = (int32_t)sceneGraph.root().children().size();
	if (rootChildren != sceneGraphRootChildren) {
		Log::error("VXA root child count doesn't match with current loaded scene graph %i vs %i", rootChildren, sceneGraphRootChildren);
		return false;
	}

	for (int32_t i = 0; i < rootChildren; ++i) {
		const int nodeId = sceneGraph.root().children()[i];
		SceneGraphNode& node = sceneGraph.node(nodeId);
		if (!recursiveImportNode(filename, stream, sceneGraph, node, animId)) {
			Log::error("VXA: failed to import children");
			return false;
		}
	}
	return true;
}

bool VXAFormat::saveRecursiveNode(const SceneGraph& sceneGraph, const SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream) {
	const SceneGraphKeyFrames &kfs = node.keyFrames();
	wrapBool(stream.writeUInt32(kfs.size()))
	for (const SceneGraphKeyFrame &kf : kfs) {
		wrapBool(stream.writeInt32(kf.frame))
		const int interpolation = vxa_priv::getInterpolationType(kf.interpolation);
		if (interpolation == -1) {
			Log::error("Could not find valid interpolation mapping for %i", (int)kf.interpolation);
			return false;
		}
		wrapBool(stream.writeInt32(interpolation))
		wrapBool(stream.writeBool(kf.longRotation))
		const SceneGraphTransform &transform = kf.transform();
		wrapBool(stream.writeFloat(transform.translation().x))
		wrapBool(stream.writeFloat(transform.translation().y))
		wrapBool(stream.writeFloat(transform.translation().z))
		wrapBool(stream.writeFloat(transform.localTranslation().x))
		wrapBool(stream.writeFloat(transform.localTranslation().y))
		wrapBool(stream.writeFloat(transform.localTranslation().z))
		wrapBool(stream.writeFloat(transform.orientation().x))
		wrapBool(stream.writeFloat(transform.orientation().y))
		wrapBool(stream.writeFloat(transform.orientation().z))
		wrapBool(stream.writeFloat(transform.orientation().w))
		wrapBool(stream.writeFloat(transform.localOrientation().x))
		wrapBool(stream.writeFloat(transform.localOrientation().y))
		wrapBool(stream.writeFloat(transform.localOrientation().z))
		wrapBool(stream.writeFloat(transform.localOrientation().w))
		wrapBool(stream.writeFloat(transform.scale()))
		wrapBool(stream.writeFloat(transform.localScale()))
	}
	const int32_t childCount = (int32_t)node.children().size();
	wrapBool(stream.writeInt32(childCount));
	for (int child : node.children()) {
		const voxelformat::SceneGraphNode &cnode = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, cnode, filename, stream))
	}
	return true;
}

bool VXAFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	const voxelformat::SceneGraphNode &root = sceneGraph.root();
	const voxelformat::SceneGraphNodeChildren &children = root.children();
	const int childCount = (int)children.size();
	if (childCount <= 0) {
		Log::error("Could not save VXA: Empty scene graph");
		return false;
	}

	const core::String &baseFilename = core::string::extractFilename(filename);
	const size_t idx = baseFilename.find(".");
	if (idx == core::String::npos) {
		Log::error("Unexpected filename for VXA given - no animation id found: %s", filename.c_str());
		return false;
	}
	const core::String &animationId = baseFilename.substr(idx + 1);

	wrapBool(stream.writeUInt32(FourCC('V','X','A','2')))
	uint64_t hash[2] {0};
	vxa_priv::calculateHash(sceneGraph, hash);
	wrapBool(stream.writeUInt64(hash[0]))
	wrapBool(stream.writeUInt64(hash[1]))
	wrapBool(stream.writeString(animationId.c_str(), true))
	Log::debug("Save animation %s", animationId.c_str());
	wrapBool(stream.writeInt32(1)) // root node has one child
	if (childCount != 1 || sceneGraph.node(children[0]).name() != "Controller") {
		// add controller node (see VXRFormat)
		wrapBool(stream.writeInt32(0)) // no key frames for controller node
		wrapBool(stream.writeInt32(childCount))
	}
	for (int child : children) {
		const voxelformat::SceneGraphNode &node = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, node, filename, stream))
	}
	Log::debug("Save vxa to %s", filename.c_str());
	return true;
}

#undef wrap
#undef wrapBool

}
