/**
 * @file
 */

#include "ThingFormat.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "glm/gtc/type_ptr.hpp"
#include "io/Archive.h"
#include "io/FilesystemEntry.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"

namespace voxelformat {

bool ThingFormat::loadNodeSpec(io::SeekableReadStream &stream, NodeSpec &nodeSpec) const {
	core::String nodeConfig;
	if (!stream.readString(stream.size(), nodeConfig)) {
		Log::error("ThingFormat: Failed to read node config");
		return false;
	}
	core::Tokenizer tok(nodeConfig.c_str(), nodeConfig.size(), ":");
	while (tok.hasNext()) {
		core::String token = tok.next();
		if (token == "name") {
			nodeSpec.name = tok.next();
		} else if (token == "modelName") {
			nodeSpec.modelName = tok.next();
		} else if (token == "thingLibraryId") {
			nodeSpec.thingLibraryId = tok.next();
		} else if (token == "opacity") {
			nodeSpec.opacity = core::string::toFloat(tok.next());
		} else if (token == "color") {
			core::string::parseHex(tok.next().c_str(), nodeSpec.color.r, nodeSpec.color.g, nodeSpec.color.b,
								   nodeSpec.color.a);
		} else if (token == "localPos") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localPos));
		} else if (token == "localRot") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localRot));
		} else if (token == "localSize") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localSize));
		} else {
			Log::debug("ThingFormat: Ignoring token: %s", token.c_str());
		}
	}
	return true;
}

bool ThingFormat::loadNode(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &sceneGraph,
						   const LoadContext &ctx) {
	if (nodeSpec.modelName.empty()) {
		Log::error("ThingFormat: Missing modelName in node spec");
		return false;
	}
	io::SeekableReadStreamPtr modelStream = archive->readStream(nodeSpec.modelName);
	if (!modelStream) {
		Log::error("ThingFormat: Failed to open model: %s", nodeSpec.modelName.c_str());
		return false;
	}
	scenegraph::SceneGraph voxSceneGraph;
	VoxFormat format;
	if (!format.load(nodeSpec.modelName, *modelStream.get(), voxSceneGraph, ctx)) {
		return false;
	}
	for (const auto &e : voxSceneGraph.nodes()) {
		if (!e->second.isModelNode()) {
			continue;
		}
		scenegraph::SceneGraphNode &node = voxSceneGraph.node(e->first);
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		scenegraph::SceneGraphTransform transform;
		transform.setLocalOrientation(glm::quat(glm::radians(nodeSpec.localRot)));
		transform.setLocalTranslation(nodeSpec.localPos);
		if (nodeSpec.localSize.x != 0.0f && nodeSpec.localSize.y != 0.0f && nodeSpec.localSize.z != 0.0f) {
			const glm::vec3 fullSize(node.region().getDimensionsInVoxels());
			transform.setLocalScale(nodeSpec.localSize / fullSize);
		}
		node.setTransform(keyFrameIdx, transform);
		node.setColor(nodeSpec.color);
		node.setProperty("thingLibraryId", nodeSpec.thingLibraryId);
		palette::Palette &palette = node.palette();
		for (size_t i = 0; i < palette.size(); ++i) {
			core::RGBA rgba = palette.color(i);
			const float alpha = 255.0f * nodeSpec.opacity;
			rgba.a = (uint8_t)alpha;
			palette.setColor(i, rgba);
		}
	}
	return !scenegraph::copySceneGraph(sceneGraph, voxSceneGraph).empty();
}

bool ThingFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream,
							 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	if (!io::isSupportedArchive(filename)) {
		Log::error("ThingFormat: Unsupported archive format: %s", filename.c_str());
		return false;
	}
	io::ArchivePtr archive = io::openArchive(io::filesystem(), filename, &stream);
	if (!archive) {
		Log::error("ThingFormat: Failed to open archive: %s", filename.c_str());
		return false;
	}

	const io::ArchiveFiles &files = archive->files();
	for (const io::FilesystemEntry &file : files) {
		if (file.isDirectory()) {
			continue;
		}
		if (core::string::extractExtension(file.name) == "node") {
			if (io::SeekableReadStreamPtr nodeSpecStream = archive->readStream(file.fullPath)) {
				NodeSpec nodeSpec;
				if (!loadNodeSpec(*nodeSpecStream.get(), nodeSpec)) {
					Log::error("ThingFormat: Failed to load node spec: %s", file.name.c_str());
					return false;
				}
				if (!loadNode(archive, nodeSpec, sceneGraph, ctx)) {
					Log::error("ThingFormat: Failed to load node: %s", file.name.c_str());
					return false;
				}
			}
		}
	}
	return true;
}

bool ThingFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							 io::SeekableWriteStream &stream, const SaveContext &ctx) {
	// filename 000000XX.node
#if 0
name: "<name>"
modelName: "<voxfile.vox>"
localPos: "0, 0, 0"
localRot: "0, 180, 0"
localSize: "13, 10, 13"
color: "#ffffff"
opacity: 1
texName: ""
texTiling: 0
mediaName: ""
flags: 0
physicsType: 0
script: ""
hNodeOfOriginal: "0x00000000"
onClickLuaSnippet: ""
thingLibraryId: "<someid-skip-it>"
renderSpec: {
  glowThresh: 0.5
  glowIntensity: 0
}
animSpec: {
  mode: 1
  startFrame: 0
  endFrame: -1
  fps: 4
  pause: 0
}
#endif
	return false;
}

} // namespace voxelformat
