/**
 * @file
 */

#include "VXAFormat.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/MD5.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "io/StreamUtil.h"
#include "io/StreamUtilQuat.h"
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

struct ChannelKeyFrame {
	int32_t frame;
	int32_t interpolation;
	float val;
};

// Interpolate a channel's value at a given frame from its explicit keyframes.
// Uses the nearest keyframe value if the frame is before/after all keyframes,
// or linearly interpolates between the two surrounding keyframes.
static float interpolateChannelValue(const core::DynamicArray<ChannelKeyFrame> &channel, int32_t frame) {
	if (channel.empty()) {
		return 0.0f;
	}

	const size_t n = channel.size();

	// Exact match
	for (size_t i = 0; i < n; ++i) {
		if (channel[i].frame == frame) {
			return channel[i].val;
		}
	}

	// Before first keyframe
	if (frame <= channel[0].frame) {
		return channel[0].val;
	}

	// After last keyframe
	if (frame >= channel[n - 1].frame) {
		return channel[n - 1].val;
	}

	// Linear interpolation between surrounding keyframes
	for (size_t i = 0; i < n - 1; ++i) {
		if (channel[i].frame <= frame && frame <= channel[i + 1].frame) {
			const int32_t range = channel[i + 1].frame - channel[i].frame;
			if (range == 0) {
				return channel[i].val;
			}
			const float t = (float)(frame - channel[i].frame) / (float)range;
			return channel[i].val + t * (channel[i + 1].val - channel[i].val);
		}
	}

	return channel[n - 1].val;
}

} // namespace vxa_priv

bool VXAFormat::recursiveImportNodeSince3(const core::String &filename, io::SeekableReadStream &stream,
										  scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
										  const core::String &animId, int version) {
	// VXA3/4 stores 7 independent channels, each with its own set of keyframes:
	// channel 0-2: position (x, y, z)
	// channel 3-5: rotation (euler angles in radians)
	// channel 6: local scale
	// Different channels may have keyframes at different frames. We must read all
	// channel data first, then merge them by interpolating missing channel values
	// at each unique frame to avoid defaulting to zero.
	core::DynamicArray<vxa_priv::ChannelKeyFrame> channelData[7];

	for (int channel = 0; channel < 7; ++channel) {
		int32_t keyFrameCount;
		wrap(stream.readInt32(keyFrameCount))
		Log::debug("Channel %i: %i keyframes", channel, keyFrameCount);

		for (int kf = 0; kf < keyFrameCount; ++kf) {
			int32_t frameIdx;
			wrap(stream.readInt32(frameIdx))

			if (channel == 6 && frameIdx > 0) {
				// scale keyframes beyond frame 0 are skipped (VoxEdit convention)
				int32_t dummy;
				wrap(stream.readInt32(dummy))
				float dummyf;
				wrap(stream.readFloat(dummyf))
				continue;
			}

			vxa_priv::ChannelKeyFrame ckf;
			ckf.frame = frameIdx;
			wrap(stream.readInt32(ckf.interpolation))
			if (channel == 3) {
				/* bool slerp =*/stream.readBool(); // TODO: VOXELFORMAT: animation
			}
			wrap(stream.readFloat(ckf.val))
			channelData[channel].push_back(ckf);
		}
	}

	// Collect all unique frame indices across all channels
	core::Buffer<int32_t> allFrames;
	for (int ch = 0; ch < 7; ++ch) {
		for (const vxa_priv::ChannelKeyFrame &ckf : channelData[ch]) {
			bool found = false;
			for (const int32_t &f : allFrames) {
				if (f == ckf.frame) {
					found = true;
					break;
				}
			}
			if (!found) {
				allFrames.push_back(ckf.frame);
			}
		}
	}

	allFrames.sort(core::Less<int32_t>());

	// For each unique frame, create a keyframe with values from all channels.
	// If a channel doesn't have an explicit keyframe at this frame, interpolate
	// from the channel's surrounding keyframes.
	for (const int32_t frameIdx : allFrames) {
		scenegraph::KeyFrameIndex keyFrameIdx = node.addKeyFrame(frameIdx);
		if (keyFrameIdx == InvalidKeyFrame) {
			keyFrameIdx = node.keyFrameForFrame(frameIdx);
		}
		scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
		keyFrame.frameIdx = frameIdx;

		// Use the interpolation type from the first channel that has an explicit keyframe here
		bool interpSet = false;
		for (int ch = 0; ch < 7 && !interpSet; ++ch) {
			for (const vxa_priv::ChannelKeyFrame &ckf : channelData[ch]) {
				if (ckf.frame == frameIdx) {
					if (ckf.interpolation == -1) {
						keyFrame.interpolation = scenegraph::InterpolationType::Linear;
					} else if (ckf.interpolation < 0 || ckf.interpolation >= lengthof(vxa_priv::interpolationTypes)) {
						keyFrame.interpolation = scenegraph::InterpolationType::Linear;
						Log::warn("Could not find a supported easing type for %i (%s)", ckf.interpolation, filename.c_str());
					} else {
						keyFrame.interpolation = vxa_priv::interpolationTypes[ckf.interpolation];
					}
					interpSet = true;
					break;
				}
			}
		}

		glm::vec3 translation(0.0f);
		glm::vec3 eulerAngles(0.0f);
		float scale = 1.0f;

		for (int ch = 0; ch < 7; ++ch) {
			float val = vxa_priv::interpolateChannelValue(channelData[ch], frameIdx);
			if (ch <= 2) {
				translation[ch] = val;
			} else if (ch <= 5) {
				eulerAngles[ch - 3] = val;
			} else {
				scale = val;
			}
		}

		scenegraph::SceneGraphTransform &transform = keyFrame.transform();
		transform.setLocalTranslation(translation);
		transform.setLocalOrientation(glm::quat(eulerAngles));
		transform.setLocalScale(glm::vec3(scale));
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
		transform.setLocalTranslation(localTranslation);
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

void VXAFormat::applyPivotFixV1(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node) {
	for (int childId : node.children()) {
		scenegraph::SceneGraphNode &child = sceneGraph.node(childId);
		// the parent node must have a model volume for pivot correction
		if (node.isAnyModelNode() && node.volume() != nullptr) {
			const glm::vec3 volumesize = node.region().getDimensionsInVoxels();
			const glm::vec3 pivotTranslation = (node.pivot() * 2.0f - 1.0f) * 0.5f * volumesize;
			scenegraph::SceneGraphKeyFrames *kfs = child.keyFrames();
			if (kfs != nullptr) {
				for (scenegraph::SceneGraphKeyFrame &kf : *kfs) {
					scenegraph::SceneGraphTransform &transform = kf.transform();
					transform.setLocalTranslation(transform.localTranslation() - pivotTranslation);
				}
			}
		}
		applyPivotFixV1(sceneGraph, child);
	}
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

	if (version > 4) { // version 4 is the same as version 3
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
	if (version == 1) {
		// VXA version 1 positions are relative to the parent model's center rather than
		// its pivot. Apply the pivot correction using the parent's model pivot.
		applyPivotFixV1(sceneGraph, sceneGraph.node(0));
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
