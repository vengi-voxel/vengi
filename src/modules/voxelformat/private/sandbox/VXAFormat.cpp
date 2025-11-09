/**
 * @file
 */

#include "VXAFormat.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/MD5.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "io/StreamUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load vxa file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load vxa file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

namespace vxa_priv {
static const scenegraph::InterpolationType interpolationTypes[]{
	scenegraph::InterpolationType::Instant,		  scenegraph::InterpolationType::Linear,
	scenegraph::InterpolationType::QuadEaseIn,	  scenegraph::InterpolationType::QuadEaseOut,
	scenegraph::InterpolationType::QuadEaseInOut, scenegraph::InterpolationType::CubicEaseIn,
	scenegraph::InterpolationType::CubicEaseOut,  scenegraph::InterpolationType::CubicEaseInOut,
};

static void addNodeToHashStream_r(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								  io::WriteStream &stream) {
	stream.writeString(node.name(), false);
	const core::String &childHex = core::string::toHex((int32_t)node.children().size()).toUpper();
	stream.writeString(childHex, false);
	for (int child : node.children()) {
		addNodeToHashStream_r(sceneGraph, sceneGraph.node(child), stream);
	}
}

static void calculateHash(const scenegraph::SceneGraph &sceneGraph, uint64_t hash[2]) {
	io::BufferedReadWriteStream stream(4096);
	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	const scenegraph::SceneGraphNodeChildren &children = root.children();

	const int childCount = (int)children.size();
	if (childCount != 1 || sceneGraph.node(children[0]).name() != SANDBOX_CONTROLLER_NODE) {
		// add controller node (see VXRFormat)
		stream.writeString(SANDBOX_CONTROLLER_NODE, false);
		stream.writeString(core::string::toHex(childCount).toUpper(), false);
	}
	for (int child : children) {
		const scenegraph::SceneGraphNode &node = sceneGraph.node(child);
		addNodeToHashStream_r(sceneGraph, node, stream);
	}
	uint8_t digest[16];
	core::md5sum(stream.getBuffer(), (uint32_t)stream.size(), digest);
	io::MemoryReadStream md5stream(digest, sizeof(digest));
	md5stream.readUInt64(hash[0]);
	md5stream.readUInt64(hash[1]);
	Log::debug("hash: %s", core::md5ToString(digest).c_str());
}

static int getInterpolationType(scenegraph::InterpolationType type) {
	for (int i = 0; i < lengthof(interpolationTypes); ++i) {
		if (interpolationTypes[i] == type) {
			return i;
		}
	}
	// ignore
	return -1;
}

} // namespace vxa_priv

bool VXAFormat::recursiveImportNodeSince3(const core::String &filename, io::SeekableReadStream &stream,
										  scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
										  const core::String &animId, int version) {
	// channel 0-2 position (float)
	// channel 3-5 rotation (euler angles in radians)
	// channel 6 local scale (float)
	for (int channel = 0; channel < 7; ++channel) {
		int32_t keyFrameCount;
		wrap(stream.readInt32(keyFrameCount))
		Log::debug("Found %i keyframes", keyFrameCount);

		for (int kf = 0; kf < keyFrameCount; ++kf) {
			scenegraph::FrameIndex frameIdx;
			wrap(stream.readInt32(frameIdx))
			// max frames are 720 for vxa
			if (channel == 6 && frameIdx > 0) {
				int32_t interpolationIgnored;
				wrap(stream.readInt32(interpolationIgnored))
				float valIgnored;
				wrap(stream.readFloat(valIgnored))
				continue;
			}

			scenegraph::KeyFrameIndex keyFrameIdx = node.addKeyFrame(frameIdx);
			if (keyFrameIdx == InvalidKeyFrame) {
				keyFrameIdx = node.keyFrameForFrame(frameIdx);
			}
			scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
			keyFrame.frameIdx = frameIdx;
			int32_t interpolation;
			wrap(stream.readInt32(interpolation))
			if (interpolation == -1) {
				keyFrame.interpolation = scenegraph::InterpolationType::Linear;
			} else if (interpolation < 0 || interpolation >= lengthof(vxa_priv::interpolationTypes)) {
				keyFrame.interpolation = scenegraph::InterpolationType::Linear;
				Log::warn("Could not find a supported easing type for %i (%s)", interpolation, filename.c_str());
			} else {
				keyFrame.interpolation = vxa_priv::interpolationTypes[interpolation];
			}
			if (channel == 3) {
				/* bool slerp =*/stream.readBool(); // TODO: VOXELFORMAT: animation
			}

			float val;
			wrap(stream.readFloat(val))

			scenegraph::SceneGraphTransform &transform = keyFrame.transform();
			if (channel == 6) {
				transform.setLocalScale(glm::vec3(val));
			} else if (channel <= 2) {
				glm::vec3 translation = transform.localTranslation();
				translation[channel] = val;
				transform.setLocalTranslation(translation);
			} else if (channel > 2) {
				glm::quat orientation = transform.localOrientation();
				orientation[channel - 3] = val;
				transform.setLocalOrientation(orientation);
			}
		}
	}

	for (scenegraph::SceneGraphKeyFrame &keyFrame : node.keyFrames(animId)) {
		const glm::quat &tempAngles = keyFrame.transform().localOrientation();
		const glm::vec3 eulerAngles(tempAngles[0], tempAngles[1], tempAngles[2]);
		const glm::quat localOrientation(eulerAngles);
		keyFrame.transform().setLocalOrientation(localOrientation);
	}

	int32_t children;
	wrap(stream.readInt32(children))
	if (children != (int32_t)node.children().size()) {
		Log::error("Child count mismatch between loaded node %i and the vxa (%i/%i) (name: %s, version: %i)", node.id(), children,
				   (int)node.children().size(), node.name().c_str(), version);
		return false;
	}
	for (const int nodeId : node.children()) {
		scenegraph::SceneGraphNode &childNode = sceneGraph.node(nodeId);
		wrapBool(recursiveImportNodeSince3(filename, stream, sceneGraph, childNode, animId, version))
	}
	return true;
}

bool VXAFormat::recursiveImportNodeBefore3(const core::String &filename, io::SeekableReadStream &stream,
										   scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
										   const core::String &animId, int version) {
	scenegraph::KeyFrameIndex keyFrameCount;
	wrap(stream.readInt32(keyFrameCount))
	Log::debug("Found %u keyframes in node %s", keyFrameCount, node.name().c_str());

	if (keyFrameCount > 0) {
		// allocate all key frames
		node.keyFrame(keyFrameCount - 1);
	}
	for (scenegraph::KeyFrameIndex keyFrameIdx = 0u; keyFrameIdx < keyFrameCount; ++keyFrameIdx) {
		scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
		wrap(stream.readInt32(keyFrame.frameIdx))
		int32_t interpolation;
		wrap(stream.readInt32(interpolation))
		if (interpolation < 0 || interpolation >= lengthof(vxa_priv::interpolationTypes)) {
			keyFrame.interpolation = scenegraph::InterpolationType::Linear;
			Log::warn("Could not find a supported easing type for %i", interpolation);
		} else {
			keyFrame.interpolation = vxa_priv::interpolationTypes[interpolation];
		}
		keyFrame.longRotation = stream.readBool();

		scenegraph::SceneGraphTransform &transform = keyFrame.transform();

		glm::vec3 localTranslation;
		glm::quat localOrientation;
		float localScale;
		glm::vec3 ignoredTranslation;
		glm::quat ignoredOrientation;
		float ignoredScale;

		wrapBool(io::readVec3(stream, localTranslation))
		wrapBool(io::readVec3(stream, ignoredTranslation))
		wrapBool(io::readQuat(stream, localOrientation))
		wrapBool(io::readQuat(stream, ignoredOrientation))
		wrap(stream.readFloat(localScale))
		wrap(stream.readFloat(ignoredScale))

		transform.setLocalScale(glm::vec3(localScale));
		transform.setLocalOrientation(localOrientation);
		if (version == 1) {
			// version 1 needs to correct its translation by the pivot translation
			const glm::vec3 volumesize = node.region().getDimensionsInVoxels();
			const glm::vec3 pivotTranslation = (node.pivot() * 2.0f - 1.0f) * 0.5f * volumesize;
			transform.setLocalTranslation(localTranslation - pivotTranslation);
		} else {
			transform.setLocalTranslation(localTranslation);
		}
	}
	int32_t children;
	wrap(stream.readInt32(children))
	if (children != (int32_t)node.children().size()) {
		Log::error("Child count mismatch between loaded node %i and the vxa (%i/%i) (name: %s, version: %i)", node.id(), children,
				   (int)node.children().size(), node.name().c_str(), version);
		return false;
	}
	for (int32_t i = 0; i < children; ++i) {
		const int nodeId = node.children()[i];
		scenegraph::SceneGraphNode &cnode = sceneGraph.node(nodeId);
		wrapBool(recursiveImportNodeBefore3(filename, stream, sceneGraph, cnode, animId, version))
	}

	return true;
}

bool VXAFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint8_t magic[4];
	wrap(stream->readUInt8(magic[0]))
	wrap(stream->readUInt8(magic[1]))
	wrap(stream->readUInt8(magic[2]))
	wrap(stream->readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'A') {
		Log::error("Could not load vxa file: Invalid magic found (%c%c%c%c)", magic[0], magic[1], magic[2], magic[3]);
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

	if (version > 3) {
		Log::error("Could not load vxa file: Unsupported version found (%i)", version);
		return false;
	}

	if (sceneGraph.empty()) {
		Log::error("No previously loaded vxm/vxr nodes found");
		return false;
	}

	uint64_t md5[2];
	wrap(stream->readUInt64(md5[0]))
	wrap(stream->readUInt64(md5[1]))

	uint64_t hash[2]{0};
	vxa_priv::calculateHash(sceneGraph, hash);

	if (core_memcmp(md5, hash, sizeof(hash))) {
		// this changed between versions - uses iso8859-1 for node names and might also skip decorative nodes (stored as node property)
		Log::debug("hash checksums differ from vxa to current scene graph nodes (version: %i)", version);
	}

	char animId[1024];
	wrapBool(stream->readString(sizeof(animId), animId, true))
	Log::debug("anim: '%s'", animId);
	int32_t rootChildren = 0;
	wrap(stream->readInt32(rootChildren))
	Log::debug("rootChildren: %i", rootChildren);
	if (rootChildren == 0) {
		Log::debug("No children node found in vxa - positioning might be wrong");
		return true;
	}

	sceneGraph.node(0).setProperty("vxaversion", core::string::toString(version));
	sceneGraph.addAnimation(animId);
	sceneGraph.setAnimation(animId);

	const int32_t sceneGraphRootChildren = (int32_t)sceneGraph.root().children().size();
	if (rootChildren != sceneGraphRootChildren) {
		Log::error("VXA root child count doesn't match with current loaded scene graph %i vs %i", rootChildren,
				   sceneGraphRootChildren);
		return false;
	}

	for (int32_t i = 0; i < rootChildren; ++i) {
		const int nodeId = sceneGraph.root().children()[i];
		scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
		if (version <= 2) {
			if (!recursiveImportNodeBefore3(filename, *stream, sceneGraph, node, animId, version)) {
				Log::error("VXA: failed to import children for version %i", version);
				return false;
			}
		} else {
			if (!recursiveImportNodeSince3(filename, *stream, sceneGraph, node, animId, version)) {
				Log::error("VXA: failed to import children for version %i", version);
				return false;
			}
		}
	}
	sceneGraph.updateTransforms();
	return true;
}

bool VXAFormat::saveRecursiveNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								  const core::String &animation, const core::String &filename,
								  io::SeekableWriteStream &stream) {
	const scenegraph::SceneGraphKeyFrames &kfs = node.keyFrames(animation);
	wrapBool(stream.writeUInt32((uint32_t)kfs.size()))
	for (const scenegraph::SceneGraphKeyFrame &kf : kfs) {
		wrapBool(stream.writeInt32(kf.frameIdx))
		const int interpolation = vxa_priv::getInterpolationType(kf.interpolation);
		wrapBool(stream.writeInt32(interpolation))
		wrapBool(stream.writeBool(kf.longRotation))
		const scenegraph::SceneGraphTransform &transform = kf.transform();
		wrapBool(stream.writeFloat(transform.worldTranslation().x))
		wrapBool(stream.writeFloat(transform.worldTranslation().y))
		wrapBool(stream.writeFloat(transform.worldTranslation().z))
		wrapBool(stream.writeFloat(transform.localTranslation().x))
		wrapBool(stream.writeFloat(transform.localTranslation().y))
		wrapBool(stream.writeFloat(transform.localTranslation().z))
		wrapBool(stream.writeFloat(transform.worldOrientation().x))
		wrapBool(stream.writeFloat(transform.worldOrientation().y))
		wrapBool(stream.writeFloat(transform.worldOrientation().z))
		wrapBool(stream.writeFloat(transform.worldOrientation().w))
		wrapBool(stream.writeFloat(transform.localOrientation().x))
		wrapBool(stream.writeFloat(transform.localOrientation().y))
		wrapBool(stream.writeFloat(transform.localOrientation().z))
		wrapBool(stream.writeFloat(transform.localOrientation().w))
		wrapBool(stream.writeFloat(transform.worldScale().x)) // TODO: VOXELFORMAT: vxa only support uniform scales
		wrapBool(stream.writeFloat(transform.localScale().x)) // TODO: VOXELFORMAT: vxa only support uniform scales
	}
	const int32_t childCount = (int32_t)node.children().size();
	wrapBool(stream.writeInt32(childCount));
	for (int child : node.children()) {
		const scenegraph::SceneGraphNode &cnode = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, cnode, animation, filename, stream))
	}
	return true;
}

bool VXAFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode &root = sceneGraph.root();
	const scenegraph::SceneGraphNodeChildren &children = root.children();
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

	wrapBool(stream->writeUInt32(FourCC('V', 'X', 'A', '2')))
	uint64_t hash[2]{0};
	vxa_priv::calculateHash(sceneGraph, hash);
	wrapBool(stream->writeUInt64(hash[0]))
	wrapBool(stream->writeUInt64(hash[1]))
	wrapBool(stream->writeString(animationId.c_str(), true))
	Log::debug("Save animation %s", animationId.c_str());
	wrapBool(stream->writeInt32(1)) // root node has one child
	if (childCount != 1 || sceneGraph.node(children[0]).name() != SANDBOX_CONTROLLER_NODE) {
		// add controller node (see VXRFormat)
		wrapBool(stream->writeInt32(0)) // no key frames for controller node
		wrapBool(stream->writeInt32(childCount))
	}
	for (int child : children) {
		const scenegraph::SceneGraphNode &node = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, node, animationId, filename, *stream))
	}
	Log::debug("Save vxa to %s", filename.c_str());
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
