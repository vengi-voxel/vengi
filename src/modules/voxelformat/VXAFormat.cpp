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

static const InterpolationType interpolationTypes[]{
	InterpolationType::Instant,		 InterpolationType::Linear,			InterpolationType::QuadEaseIn,
	InterpolationType::QuadEaseOut,	 InterpolationType::QuadEaseInOut,	InterpolationType::CubicEaseIn,
	InterpolationType::CubicEaseOut, InterpolationType::CubicEaseInOut,
};

bool VXAFormat::recursiveImportNode(const core::String &filename, io::SeekableReadStream &stream,
									SceneGraph &sceneGraph, SceneGraphNode& node, const core::String &animId) {
	int32_t keyFrameCount;
	wrap(stream.readInt32(keyFrameCount))
	Log::debug("Found %i keyframes", keyFrameCount);
	for (int32_t i = 0u; i < keyFrameCount; ++i) {
		SceneGraphKeyFrame &keyFrame = node.keyFrame(i);
		wrap(stream.readUInt32(keyFrame.frame))
		int32_t interpolation;
		wrap(stream.readInt32(interpolation))
		if (interpolation < 0 || interpolation >= lengthof(interpolationTypes)) {
			keyFrame.interpolation = InterpolationType::Linear;
			Log::warn("Could not find a supported easing type for %i", interpolation);
		} else {
			keyFrame.interpolation = interpolationTypes[interpolation];
		}
		keyFrame.longRotation = stream.readBool();

		glm::vec3 localPosition{0.0f};
		glm::quat localRot{0.0f, 0.0f, 0.0f, 0.0f};
		float localScale = 1.0f;
		glm::vec3 translation;
		wrap(stream.readFloat(translation.x))
		wrap(stream.readFloat(translation.y))
		wrap(stream.readFloat(translation.z))
		keyFrame.transform.setTranslation(translation);
		wrap(stream.readFloat(localPosition.x))
		wrap(stream.readFloat(localPosition.y))
		wrap(stream.readFloat(localPosition.z))
		glm::quat orientation;
		wrap(stream.readFloat(orientation.x))
		wrap(stream.readFloat(orientation.y))
		wrap(stream.readFloat(orientation.z))
		wrap(stream.readFloat(orientation.w))
		keyFrame.transform.setOrientation(orientation);
		wrap(stream.readFloat(localRot.x))
		wrap(stream.readFloat(localRot.y))
		wrap(stream.readFloat(localRot.z))
		wrap(stream.readFloat(localRot.w))
		float scale;
		wrap(stream.readFloat(scale))
		keyFrame.transform.setScale(scale);
		wrap(stream.readFloat(localScale))
		keyFrame.transform.update();
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

static int getInterpolationType(InterpolationType type) {
	for (int i = 0; i < lengthof(interpolationTypes); ++i) {
		if (interpolationTypes[i] == type) {
			return i;
		}
	}
	return -1;
}

bool VXAFormat::saveRecursiveNode(const SceneGraph& sceneGraph, const SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream) {
	const SceneGraphKeyFrames &kfs = node.keyFrames();
	wrapBool(stream.writeUInt32(kfs.size()))
	for (const SceneGraphKeyFrame &kf : kfs) {
		wrapBool(stream.writeInt32(kf.frame))
		const int interpolation = getInterpolationType(kf.interpolation);
		if (interpolation == -1) {
			Log::error("Could not find valid interpolation mapping for %i", (int)kf.interpolation);
		}
		wrapBool(stream.writeInt32(interpolation))
		wrapBool(stream.writeBool(kf.longRotation))
		wrapBool(stream.writeFloat(kf.transform.translation().x))
		wrapBool(stream.writeFloat(kf.transform.translation().y))
		wrapBool(stream.writeFloat(kf.transform.translation().z))
		wrapBool(stream.writeFloat(0.0f)) // localPosition.x
		wrapBool(stream.writeFloat(0.0f)) // localPosition.y
		wrapBool(stream.writeFloat(0.0f)) // localPosition.z
		wrapBool(stream.writeFloat(kf.transform.orientation().x))
		wrapBool(stream.writeFloat(kf.transform.orientation().y))
		wrapBool(stream.writeFloat(kf.transform.orientation().z))
		wrapBool(stream.writeFloat(kf.transform.orientation().w))
		wrapBool(stream.writeFloat(0.0f)) // localRotation.X
		wrapBool(stream.writeFloat(0.0f)) // localRotation.Y
		wrapBool(stream.writeFloat(0.0f)) // localRotation.Z
		wrapBool(stream.writeFloat(1.0f)) // localRotation.W
		wrapBool(stream.writeFloat(kf.transform.scale()))
		wrapBool(stream.writeFloat(1.0f))
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
	uint64_t md5[2] {0}; // TODO calculate me
	wrapBool(stream.writeUInt64(md5[0]))
	wrapBool(stream.writeUInt64(md5[1]))
	wrapBool(stream.writeString(animationId.c_str(), true))
	wrapBool(stream.writeInt32(1))
	if (childCount != 1 || sceneGraph.node(children[0]).name() != "Controller") {
		// add controller node (see VXRFormat)
		wrapBool(stream.writeInt32(0)) // no key frames for controller node
		wrapBool(stream.writeInt32(childCount))
	}
	for (int child : children) {
		const voxelformat::SceneGraphNode &node = sceneGraph.node(child);
		wrapBool(saveRecursiveNode(sceneGraph, node, filename, stream))
	}
	return false;
}

#undef wrap
#undef wrapBool

}
