/**
 * @file
 */

#include "HVAFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "io/Archive.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/commandconquer/VXLShared.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

bool HVAFormat::readHVAHeader(io::SeekableReadStream &stream, vxl::HVAHeader &header) const {
	char name[16];
	wrapBool(stream.readString(lengthof(name), name, false))
	header.filename = name;
	Log::debug("hva name: %s", header.filename.c_str());
	wrap(stream.readUInt32(header.numFrames))
	Log::debug("numframes: %i", header.numFrames);
	wrap(stream.readUInt32(header.numLayers))
	Log::debug("sections: %i", header.numLayers);
	for (uint32_t i = 0; i < header.numLayers; ++i) {
		wrapBool(stream.readString(lengthof(name), name, false))
		header.nodeNames[i] = name;
		Log::debug("hva section %u: %s", i, header.nodeNames[i].c_str());
	}
	return true;
}

bool HVAFormat::readHVAFrames(io::SeekableReadStream &stream, const vxl::VXLModel &mdl, vxl::HVAModel &file) const {
	if (file.header.numLayers >= lengthof(file.frames)) {
		Log::error("Max allowed frame count exceeded");
		return false;
	}
	for (uint32_t i = 0; i < file.header.numLayers; ++i) {
		file.header.layerIds[i] = mdl.findLayerByName(file.header.nodeNames[i]);
		if (file.header.layerIds[i] == -1) {
			Log::debug("Failed to resolve layer id for '%s' (node idx: %i/%i)", file.header.nodeNames[i].c_str(), i,
					   file.header.numLayers);
			for (uint32_t j = 0; j < mdl.header.layerCount; ++j) {
				Log::debug(" - found: %s", mdl.layerHeaders[j].name);
			}
		}
	}

	for (uint32_t frameIdx = 0; frameIdx < file.header.numFrames; ++frameIdx) {
		vxl::HVAFrames &frame = file.frames[frameIdx];
		frame.resize(file.header.numLayers);
		for (uint32_t nodeIdx = 0; nodeIdx < file.header.numLayers; ++nodeIdx) {
			vxl::VXLMatrix &vxlMatrix = frame[nodeIdx];
			for (int i = 0; i < 12; ++i) {
				const int col = i % 4;
				const int row = i / 4;
				float &val = vxlMatrix.matrix[col][row];
				wrap(stream.readFloat(val))
			}
			Log::debug("load frame %u for layer %i with translation: %f:%f:%f", frameIdx, nodeIdx,
					   vxlMatrix.matrix[3][0], vxlMatrix.matrix[3][1], vxlMatrix.matrix[3][2]);
		}
	}

	return true;
}

bool HVAFormat::loadHVA(const core::String &filename, const io::ArchivePtr &archive, const vxl::VXLModel &mdl, scenegraph::SceneGraph &sceneGraph) {
	vxl::HVAModel file;
	{
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			// if there is no hva file, we still don't show an error
			return true;
		}
		wrapBool(readHVAHeader(*stream, file.header));
		wrapBool(readHVAFrames(*stream, mdl, file));
	}
	Log::debug("load %u frames", file.header.numFrames);
	for (uint32_t keyFrameIdx = 0; keyFrameIdx < file.header.numFrames; ++keyFrameIdx) {
		const vxl::HVAFrames &sectionMatrices = file.frames[keyFrameIdx];
		for (uint32_t vxlNodeId = 0; vxlNodeId < file.header.numLayers; ++vxlNodeId) {
			const core::String &name = file.header.nodeNames[vxlNodeId];
			scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName(name);
			if (node == nullptr) {
				Log::warn("Can't find node with name '%s' for vxl node %u", name.c_str(), vxlNodeId);
				continue;
			}
			Log::debug("add key frame for node '%s' at idx %u", name.c_str(), keyFrameIdx);
			// hva transforms are overriding the vxl transform
			scenegraph::SceneGraphKeyFrame &kf = node->keyFrame(keyFrameIdx);
			kf.frameIdx = keyFrameIdx * 6; // running at 6 fps

			const int nodeId = file.header.layerIds[vxlNodeId];
			if (nodeId != InvalidNodeId) {
				const vxl::VXLLayerInfo &footer = mdl.layerInfos[nodeId];
				scenegraph::SceneGraphTransform transform;
				transform.setLocalMatrix(convertHVARead(sectionMatrices[vxlNodeId], footer));
				kf.setTransform(transform);
			} else {
				Log::error("Failed to assign key frame idx %u to node '%s'", keyFrameIdx, name.c_str());
			}
		}
	}
	return true;
}

bool HVAFormat::writeHVAHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const {
	char name[16];
	core_memset(name, 0, sizeof(name));
	// TODO: VOXELFORMAT: name
	if (stream.write(name, sizeof(name)) == -1) {
		Log::error("Failed to write hva header name");
		return false;
	}
	uint32_t numFrames = 0;

	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		numFrames = core_max(numFrames, node.keyFrames().size());
	}

	stream.writeUInt32(numFrames);
	uint32_t numNodes = sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels);
	stream.writeUInt32(numNodes);
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const core::String &nodeNamePart = node.name().substr(0, 15);
		if (stream.write(nodeNamePart.c_str(), nodeNamePart.size()) == -1) {
			Log::error("Failed to write layer name");
			return false;
		}
		for (size_t i = 0; i < 16 - nodeNamePart.size(); ++i) {
			wrapBool(stream.writeUInt8(0))
		}
	}
	return true;
}

bool HVAFormat::writeHVAFrames(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const {
	uint32_t numFrames = 0;
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		numFrames = core_max(numFrames, node.keyFrames().size());
	}

	for (uint32_t i = 0; i < numFrames; ++i) {
		for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
			const scenegraph::SceneGraphNode &node = *iter;
			const scenegraph::SceneGraphTransform &transform = node.transform(i);
			const vxl::VXLMatrix &vxlMatrix = vxl::convertHVAWrite(transform.localMatrix());

			for (int j = 0; j < 12; ++j) {
				const int col = j % 4;
				const int row = j / 4;
				float val = vxlMatrix.matrix[col][row];
				wrapBool(stream.writeFloat(val))
			}
		}
	}
	return true;
}

bool HVAFormat::saveHVA(const core::String &filename, const io::ArchivePtr &archive, const scenegraph::SceneGraph &sceneGraph) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	wrapBool(writeHVAHeader(*stream, sceneGraph));
	wrapBool(writeHVAFrames(*stream, sceneGraph));
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
