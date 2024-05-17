/**
 * @file
 */

#include "ThingFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelutil/ImageUtils.h"

namespace voxelformat {

bool ThingFormat::loadNodeSpec(io::SeekableReadStream &stream, NodeSpec &nodeSpec) const {
	core::String nodeConfig;
	if (!stream.readString((int)stream.size(), nodeConfig)) {
		Log::error("ThingFormat: Failed to read node config");
		return false;
	}
	ThingNodeParser parser;
	return parser.parseNode(nodeConfig, nodeSpec);
}

static scenegraph::SceneGraphTransform toTransform(const voxel::Region &region, const glm::vec3 &localPos,
												   const glm::vec3 &localRot, const glm::vec3 &localSize) {
	// TODO: positioning is wrong
	scenegraph::SceneGraphTransform transform;
	transform.setLocalOrientation(glm::quat(glm::radians(localRot)));
	transform.setLocalTranslation(localPos);
	if (localSize.x != 0.0f && localSize.y != 0.0f && localSize.z != 0.0f) {
		const glm::vec3 fullSize(region.getDimensionsInVoxels());
		transform.setLocalScale(localSize / fullSize);
	}
	return transform;
}

void ThingFormat::addMediaImage(const io::ArchivePtr &archive, const NodeSpec &nodeSpec,
							scenegraph::SceneGraph &voxSceneGraph) {
	if (nodeSpec.mediaName.empty()) {
		Log::debug("No media name found");
		return;
	}
	if (!io::isImage(nodeSpec.mediaName)) {
		Log::debug("Media name is no image %s", nodeSpec.mediaName.c_str());
		return;
	}
	core::ScopedPtr<io::SeekableReadStream> mediaStream(archive->readStream(nodeSpec.mediaName));
	if (!mediaStream) {
		Log::error("ThingFormat: Failed to open media: %s", nodeSpec.mediaName.c_str());
		return;
	}
	const MediaCanvas &mediaCanvas = nodeSpec.mediaCanvas;
	const image::ImagePtr &img = image::loadImage(nodeSpec.mediaName, *mediaStream);
	if (!img || !img->isLoaded()) {
		Log::error("Failed to load image %s", nodeSpec.mediaName.c_str());
		return;
	}
	if (!img->resize(mediaCanvas.localScale.x, mediaCanvas.localScale.y)) {
		Log::error("Failed to resize image to %fx%f", mediaCanvas.localScale.x, mediaCanvas.localScale.y);
		return;
	}
	const palette::Palette &palette = voxel::getPalette();
	voxel::RawVolume *mediaPlane = voxelutil::importAsPlane(img, palette);
	if (mediaPlane) {
		scenegraph::SceneGraphNode mediaNode;
		mediaNode.setVolume(mediaPlane);
		mediaNode.setPivot(glm::vec3{0.5f});
		mediaNode.setPalette(palette);
		mediaNode.setColor(nodeSpec.color);
		mediaNode.setName(nodeSpec.mediaName);
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		const scenegraph::SceneGraphTransform &transform =
			toTransform(mediaNode.region(), mediaCanvas.localPos, mediaCanvas.localRot, mediaCanvas.localScale);
		mediaNode.setTransform(keyFrameIdx, transform);
		voxSceneGraph.emplace(core::move(mediaNode));
	} else {
		Log::error("ThingFormat: Failed to import media plane: %s", nodeSpec.mediaName.c_str());
	}
}

bool ThingFormat::loadNode(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &sceneGraph,
						   const LoadContext &ctx, int parent) {
	if (nodeSpec.modelName.empty()) {
		Log::error("ThingFormat: Missing modelName in node spec");
		return false;
	}
	scenegraph::SceneGraph voxSceneGraph;
	Log::debug("ThingFormat: Load vox file: %s", nodeSpec.modelName.c_str());
	VoxFormat format;
	if (!format.load(nodeSpec.modelName, archive, voxSceneGraph, ctx)) {
		Log::error("ThingFormat: Failed to load model: %s", nodeSpec.modelName.c_str());
		return false;
	}
	for (const auto &e : voxSceneGraph.nodes()) {
		if (!e->second.isModelNode()) {
			continue;
		}
		scenegraph::SceneGraphNode &node = voxSceneGraph.node(e->first);
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		scenegraph::SceneGraphTransform transform = toTransform(node.region(), nodeSpec.localPos, nodeSpec.localRot, nodeSpec.localSize);
		node.setTransform(keyFrameIdx, transform);
		node.setPivot(glm::vec3{0.5f});
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
	addMediaImage(archive, nodeSpec, voxSceneGraph);
	const core::DynamicArray<int> &nodes = scenegraph::copySceneGraph(sceneGraph, voxSceneGraph, parent);
	if (nodes.empty()) {
		Log::error("ThingFormat: Failed to copy the scene graph from node %s", nodeSpec.modelName.c_str());
		return false;
	}
	for (const NodeSpec &child : nodeSpec.children) {
		if (!loadNode(archive, child, sceneGraph, ctx, nodes[0])) {
			return false;
		}
	}
	return true;
}

bool ThingFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
							 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::ArchivePtr zipArchive = io::openZipArchive(stream);

	io::ArchiveFiles files;
	zipArchive->list("*.node", files);
	for (const io::FilesystemEntry &file : files) {
		core::ScopedPtr<io::SeekableReadStream> nodeSpecStream(zipArchive->readStream(file.fullPath));
		if (nodeSpecStream) {
			NodeSpec nodeSpec;
			if (!loadNodeSpec(*nodeSpecStream, nodeSpec)) {
				Log::error("ThingFormat: Failed to load node spec: %s", file.name.c_str());
				return false;
			}
			if (!loadNode(zipArchive, nodeSpec, sceneGraph, ctx)) {
				Log::error("ThingFormat: Failed to load node: %s", file.name.c_str());
				return false;
			}
		}
	}
	sceneGraph.updateTransforms();
	return true;
}

bool ThingFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							 const io::ArchivePtr &archive, const SaveContext &ctx) {
	return false;
}

} // namespace voxelformat
