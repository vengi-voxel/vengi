/**
 * @file
 */

#include "LDrawFormat.h"
#include "LegoUtil.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/CachingArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"

namespace voxelformat {

namespace {

size_t countGeometryTris(const char *const *lines, size_t lineCount) {
	size_t tris = 0;
	for (size_t i = 0; i < lineCount; ++i) {
		const char *ptr = lines[i];
		while (*ptr == ' ' || *ptr == '\t') {
			++ptr;
		}
		if (*ptr == '3') {
			tris += 1;
		} else if (*ptr == '4') {
			tris += 2;
		}
	}
	return tris;
}

} // namespace

bool LDrawFormat::parseStream(io::SeekableReadStream &stream, Mesh &mesh, legoutil::ColorMap &colors,
							  core::DynamicArray<legoutil::SubFileRef> &subFiles, core::String &name,
							  core::String &author) const {
	char line[512];
	while (stream.readLine(sizeof(line), line)) {
		legoutil::parseLine(line, mesh, colors, subFiles, name, author);
	}
	return !mesh.vertices.empty() || !subFiles.empty();
}

bool LDrawFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
								 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	const core::String ldrawDir = core::getVar(cfg::VoxformatLDrawDir)->strVal();
	Log::debug("LDraw library path: '%s'", ldrawDir.c_str());

	io::CachingArchive cachedArchive(legoutil::openLookupArchive(archive));
	legoutil::registerLdrawSearchPaths(cachedArchive);

	legoutil::ColorMap colors;
	legoutil::initColors(colors);
	legoutil::parseLDConfig(cachedArchive, colors);

	struct MpdSection {
		core::String fileName;
		core::DynamicArray<core::String> lines;
	};
	core::DynamicArray<MpdSection> mpdSections;
	bool isMpd = false;

	char line[512];
	core::DynamicArray<core::String> allLines;
	while (stream->readLine(sizeof(line), line)) {
		const char *ptr = line;
		while (*ptr == ' ' || *ptr == '\t') {
			++ptr;
		}
		if (*ptr == '0') {
			const char *meta = ptr + 1;
			while (*meta == ' ' || *meta == '\t') {
				++meta;
			}
			if (strncmp(meta, "FILE ", 5) == 0) {
				isMpd = true;
				MpdSection section;
				section.fileName = core::string::trim(core::String(meta + 5));
				mpdSections.push_back(core::move(section));
				continue;
			}
			if (strncmp(meta, "NOFILE", 6) == 0) {
				continue;
			}
		}
		if (isMpd && !mpdSections.empty()) {
			mpdSections.back().lines.push_back(core::String(line));
		} else {
			allLines.push_back(core::String(line));
		}
	}

	if (isMpd) {
		bool success = false;
		for (const MpdSection &section : mpdSections) {
			Mesh mesh;
			core::DynamicArray<legoutil::SubFileRef> subFiles;
			core::String name;
			core::String author;

			{
				core::DynamicArray<const char *> linePtrs;
				linePtrs.reserve(section.lines.size());
				for (const core::String &l : section.lines) {
					linePtrs.push_back(l.c_str());
				}
				mesh.reserveAdditionalTris(countGeometryTris(linePtrs.data(), linePtrs.size()));
			}
			for (const core::String &l : section.lines) {
				legoutil::parseLine(l.c_str(), mesh, colors, subFiles, name, author);
			}

			if (mesh.vertices.empty() && subFiles.empty()) {
				continue;
			}

			if (name.empty()) {
				name = section.fileName;
			}

			int nodeId = InvalidNodeId;
			if (!mesh.vertices.empty()) {
				nodeId = voxelizeMesh(name, sceneGraph, core::move(mesh));
				if (nodeId == InvalidNodeId) {
					continue;
				}
			} else {
				nodeId = sceneGraph.root().id();
			}

			for (const legoutil::SubFileRef &ref : subFiles) {
				Mesh brickMesh;
				if (legoutil::resolveSubFile(cachedArchive, ref, colors, brickMesh, 0) && !brickMesh.vertices.empty()) {
					voxelizeMesh(ref.filename, sceneGraph, core::move(brickMesh), nodeId);
				}
			}

			if (nodeId != InvalidNodeId) {
				scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
				if (!author.empty()) {
					node.setProperty(scenegraph::PropAuthor, author);
				}
			}
			success = true;
		}
		return success;
	}

	core::DynamicArray<legoutil::SubFileRef> subFiles;
	core::String name;
	core::String author;

	Mesh mesh;
	{
		core::DynamicArray<const char *> linePtrs;
		linePtrs.reserve(allLines.size());
		for (const core::String &l : allLines) {
			linePtrs.push_back(l.c_str());
		}
		mesh.reserveAdditionalTris(countGeometryTris(linePtrs.data(), linePtrs.size()));
	}
	for (const core::String &l : allLines) {
		legoutil::parseLine(l.c_str(), mesh, colors, subFiles, name, author);
	}

	Log::debug("Parsed LDraw file %s: %i vertices, %i sub-file references", filename.c_str(), (int)mesh.vertices.size(),
			   (int)subFiles.size());

	if (mesh.vertices.empty() && subFiles.empty()) {
		Log::error("No geometry and no sub-file references found in LDraw file %s", filename.c_str());
		return false;
	}

	int nodeId = InvalidNodeId;
	if (!mesh.vertices.empty()) {
		nodeId = voxelizeMesh(name, sceneGraph, core::move(mesh));
		if (nodeId == InvalidNodeId) {
			Log::error("Failed to voxelize LDraw mesh from %s", filename.c_str());
			return false;
		}
	} else {
		nodeId = sceneGraph.root().id();
	}

	for (const legoutil::SubFileRef &ref : subFiles) {
		Mesh brickMesh;
		if (legoutil::resolveSubFile(cachedArchive, ref, colors, brickMesh, 0) && !brickMesh.vertices.empty()) {
			voxelizeMesh(ref.filename, sceneGraph, core::move(brickMesh), nodeId);
		} else if (brickMesh.vertices.empty()) {
			Log::warn("No geometry resolved for sub-file reference: %s", ref.filename.c_str());
		}
	}

	const int modelCount = (int)sceneGraph.size(scenegraph::SceneGraphNodeType::Model);
	Log::debug("LDraw loading complete: %i model nodes in scene graph", modelCount);
	if (modelCount == 0) {
		Log::error("No models could be loaded from LDraw file %s - all %i sub-file references failed to resolve. "
				   "Check that the LDraw library path ('%s' = '%s') is correct and contains the required .dat files",
				   filename.c_str(), (int)subFiles.size(), cfg::VoxformatLDrawDir, ldrawDir.c_str());
		return false;
	}

	if (name.empty()) {
		name = core::string::extractFilenameWithExtension(filename);
	}

	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	if (!author.empty()) {
		node.setProperty(scenegraph::PropAuthor, author);
	}

	return true;
}

} // namespace voxelformat
