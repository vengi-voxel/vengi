/**
 * @file
 */

#include "ThingFormat.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "core/collection/DynamicArray.h"
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

bool ThingFormat::parseChildren(core::Tokenizer &tok, NodeSpec &nodeSpec) const {
	if (!tok.hasNext()) {
		Log::error("ThingFormat: Expected token but got nothing");
		return false;
	}
	core::String token = tok.next();
	if (token != "{") {
		Log::error("ThingFormat: Expected '{' but got: %s", token.c_str());
		return false;
	}
	token = tok.next();
	if (token != "{") {
		Log::error("ThingFormat: Expected '{' but got: %s", token.c_str());
		return false;
	}
	NodeSpec child;
	if (parseNode(tok, child)) {
		nodeSpec.children.push_back(child);
	} else {
		Log::error("ThingFormat: Failed to parse child node");
		return false;
	}
	return true;
}

bool ThingFormat::parseNode(core::Tokenizer &tok, NodeSpec &nodeSpec) const {
	int depth = 1;
	while (tok.hasNext()) {
		core::String token = tok.next();
		if (token == "{") {
			++depth;
		} else if (token == "}") {
			if (depth == 0) {
				return true;
			}
			--depth;
		} else if (token == "name") {
			nodeSpec.name = tok.next();
			Log::debug("ThingFormat: name: %s", nodeSpec.name.c_str());
		} else if (token == "modelName") {
			nodeSpec.modelName = tok.next();
			Log::debug("ThingFormat: modelName: %s", nodeSpec.modelName.c_str());
		} else if (token == "thingLibraryId") {
			nodeSpec.thingLibraryId = tok.next();
			Log::debug("ThingFormat: thingLibraryId: %s", nodeSpec.thingLibraryId.c_str());
		} else if (token == "opacity") {
			nodeSpec.opacity = core::string::toFloat(tok.next());
			Log::debug("ThingFormat: opacity: %f", nodeSpec.opacity);
		} else if (token == "children") {
			Log::debug("ThingFormat: found children");
			parseChildren(tok, nodeSpec);
		} else if (token == "color") {
			core::string::parseHex(tok.next().c_str(), nodeSpec.color.r, nodeSpec.color.g, nodeSpec.color.b,
								   nodeSpec.color.a);
			Log::debug("ThingFormat: color: %d %d %d %d", nodeSpec.color.r, nodeSpec.color.g, nodeSpec.color.b,
					   nodeSpec.color.a);
		} else if (token == "localPos") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localPos), " ,\t");
			Log::debug("ThingFormat: localPos: %f %f %f", nodeSpec.localPos.x, nodeSpec.localPos.y,
					   nodeSpec.localPos.z);
		} else if (token == "localRot") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localRot), " ,\t");
			Log::debug("ThingFormat: localRot: %f %f %f", nodeSpec.localRot.x, nodeSpec.localRot.y,
					   nodeSpec.localRot.z);
		} else if (token == "localSize") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localSize), " ,\t");
			Log::debug("ThingFormat: localSize: %f %f %f", nodeSpec.localSize.x, nodeSpec.localSize.y,
					   nodeSpec.localSize.z);
		} else {
			Log::debug("ThingFormat: Ignoring token: %s", token.c_str());
		}
	}
	return true;
}

bool ThingFormat::loadNodeSpec(io::SeekableReadStream &stream, NodeSpec &nodeSpec) const {
	core::String nodeConfig;
	if (!stream.readString(stream.size(), nodeConfig)) {
		Log::error("ThingFormat: Failed to read node config");
		return false;
	}
	core::Tokenizer tok(nodeConfig.c_str(), nodeConfig.size(), ":");
	return parseNode(tok, nodeSpec);
}

bool ThingFormat::loadNode(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &sceneGraph,
						   const LoadContext &ctx, int parent) {
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
	Log::debug("ThingFormat: Load vox file: %s", nodeSpec.modelName.c_str());
	VoxFormat format;
	if (!format.load(nodeSpec.modelName, *modelStream.get(), voxSceneGraph, ctx)) {
		return false;
	}
	for (const auto &e : voxSceneGraph.nodes()) {
		if (!e->second.isModelNode()) {
			continue;
		}
		// TODO: positioning is wrong
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
		node.setName(nodeSpec.name);
		if (!nodeSpec.thingLibraryId.empty()) {
			node.setProperty("thingLibraryId", nodeSpec.thingLibraryId);
		}
		palette::Palette &palette = node.palette();
		for (size_t i = 0; i < palette.size(); ++i) {
			core::RGBA rgba = palette.color(i);
			const float alpha = 255.0f * nodeSpec.opacity;
			rgba.a = (uint8_t)alpha;
			palette.setColor(i, rgba);
		}
	}
	voxSceneGraph.updateTransforms();
	const core::DynamicArray<int> &nodes = scenegraph::copySceneGraph(sceneGraph, voxSceneGraph, parent);
	if (nodes.empty()) {
		return false;
	}
	for (const NodeSpec &child : nodeSpec.children) {
		if (!loadNode(archive, child, sceneGraph, ctx, nodes[0])) {
			return false;
		}
	}
	return true;
}

bool ThingFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream,
							 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
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
