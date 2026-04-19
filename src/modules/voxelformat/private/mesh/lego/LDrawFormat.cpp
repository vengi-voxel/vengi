/**
 * @file
 */

#include "LDrawFormat.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/CachingArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxelformat {

void LDrawFormat::initColors(ColorMap &colors) {
	// standard LDraw colors from the official specification
	colors.put(0, color::RGBA(27, 42, 52, 255));	  // Black
	colors.put(1, color::RGBA(0, 85, 191, 255));	  // Blue
	colors.put(2, color::RGBA(35, 120, 65, 255));	  // Green
	colors.put(3, color::RGBA(0, 131, 143, 255));	  // Dark Turquoise
	colors.put(4, color::RGBA(196, 40, 27, 255));	  // Red
	colors.put(5, color::RGBA(205, 98, 152, 255));	  // Dark Pink
	colors.put(6, color::RGBA(98, 71, 50, 255));	  // Brown
	colors.put(7, color::RGBA(156, 163, 168, 255));	  // Light Grey
	colors.put(8, color::RGBA(109, 110, 92, 255));	  // Dark Grey
	colors.put(9, color::RGBA(180, 210, 227, 255));	  // Light Blue
	colors.put(10, color::RGBA(75, 159, 74, 255));	  // Bright Green
	colors.put(11, color::RGBA(85, 165, 175, 255));	  // Light Turquoise
	colors.put(12, color::RGBA(242, 112, 94, 255));	  // Salmon
	colors.put(13, color::RGBA(252, 151, 172, 255));  // Pink
	colors.put(14, color::RGBA(245, 205, 47, 255));	  // Yellow
	colors.put(15, color::RGBA(255, 255, 255, 255));  // White
	colors.put(16, color::RGBA(127, 127, 127, 255));  // Main Colour (inherited)
	colors.put(17, color::RGBA(163, 228, 215, 255));  // Light Green
	colors.put(18, color::RGBA(253, 234, 140, 255));  // Light Yellow
	colors.put(19, color::RGBA(232, 207, 161, 255));  // Tan
	colors.put(20, color::RGBA(215, 196, 230, 255));  // Light Violet
	colors.put(22, color::RGBA(129, 0, 123, 255));	  // Purple
	colors.put(23, color::RGBA(0, 51, 178, 255));	  // Dark Blue-Violet
	colors.put(24, color::RGBA(0, 0, 0, 255));		  // Edge Colour
	colors.put(25, color::RGBA(224, 152, 100, 255));  // Orange
	colors.put(26, color::RGBA(218, 133, 218, 255));  // Magenta
	colors.put(27, color::RGBA(163, 228, 40, 255));	  // Lime
	colors.put(28, color::RGBA(197, 157, 115, 255));  // Dark Tan
	colors.put(29, color::RGBA(228, 173, 200, 255));  // Bright Pink
	colors.put(33, color::RGBA(0, 30, 160, 128));	  // Trans-Dark Blue
	colors.put(34, color::RGBA(86, 229, 100, 128));	  // Trans-Green
	colors.put(36, color::RGBA(201, 26, 9, 128));	  // Trans-Red
	colors.put(40, color::RGBA(99, 95, 82, 128));	  // Trans-Black
	colors.put(41, color::RGBA(174, 233, 239, 128));  // Trans-Light Cyan
	colors.put(42, color::RGBA(192, 255, 0, 128));	  // Trans-Neon Green
	colors.put(43, color::RGBA(100, 176, 227, 128));  // Trans-Medium Blue
	colors.put(44, color::RGBA(150, 112, 159, 128));  // Trans-Dark Pink
	colors.put(46, color::RGBA(247, 241, 141, 128));  // Trans-Yellow
	colors.put(47, color::RGBA(252, 252, 252, 128));  // Trans-Clear
	colors.put(52, color::RGBA(165, 105, 200, 128));  // Trans-Purple
	colors.put(57, color::RGBA(243, 207, 155, 128));  // Trans-Neon Orange
	colors.put(68, color::RGBA(243, 183, 40, 255));	  // Very Light Orange
	colors.put(69, color::RGBA(211, 62, 159, 255));	  // Bright Reddish Lilac
	colors.put(70, color::RGBA(105, 64, 39, 255));	  // Reddish Brown
	colors.put(71, color::RGBA(163, 162, 164, 255));  // Light Bluish Grey
	colors.put(72, color::RGBA(99, 95, 97, 255));	  // Dark Bluish Grey
	colors.put(73, color::RGBA(110, 153, 201, 255));  // Medium Blue
	colors.put(74, color::RGBA(161, 196, 139, 255));  // Medium Green
	colors.put(77, color::RGBA(220, 144, 149, 255));  // Light Pink
	colors.put(78, color::RGBA(249, 197, 131, 255));  // Light Nougat
	colors.put(84, color::RGBA(204, 112, 42, 255));	  // Medium Nougat
	colors.put(85, color::RGBA(107, 50, 123, 255));	  // Medium Lilac
	colors.put(86, color::RGBA(125, 91, 56, 255));	  // Light Brown
	colors.put(89, color::RGBA(76, 145, 198, 255));	  // Blue-Violet
	colors.put(92, color::RGBA(204, 142, 104, 255));  // Nougat
	colors.put(100, color::RGBA(238, 196, 182, 255)); // Light Salmon
	colors.put(110, color::RGBA(67, 84, 163, 255));	  // Violet
	colors.put(112, color::RGBA(72, 97, 172, 255));	  // Medium Violet
	colors.put(115, color::RGBA(199, 210, 60, 255));  // Medium Lime
	colors.put(118, color::RGBA(183, 215, 213, 255)); // Aqua
	colors.put(120, color::RGBA(216, 221, 86, 255));  // Light Lime
	colors.put(125, color::RGBA(252, 172, 0, 255));	  // Light Orange
	colors.put(151, color::RGBA(108, 110, 104, 255)); // Sand Green
	colors.put(191, color::RGBA(248, 187, 61, 255));  // Flame Yellowish Orange
	colors.put(212, color::RGBA(155, 178, 239, 255)); // Light Royal Blue
	colors.put(216, color::RGBA(143, 76, 42, 255));	  // Dark Brown
	colors.put(226, color::RGBA(253, 234, 140, 255)); // Bright Light Yellow
	colors.put(232, color::RGBA(125, 187, 221, 255)); // Sky Blue
	colors.put(256, color::RGBA(33, 33, 33, 255));	  // Rubber Black
	colors.put(272, color::RGBA(32, 50, 176, 255));	  // Dark Blue
	colors.put(288, color::RGBA(39, 70, 44, 255));	  // Dark Green
	colors.put(308, color::RGBA(53, 33, 0, 255));	  // Dark Brown 2
	colors.put(320, color::RGBA(120, 0, 28, 255));	  // Dark Red
	colors.put(334, color::RGBA(189, 125, 8, 255));	  // Chrome Gold
	colors.put(335, color::RGBA(224, 224, 224, 255)); // Sand Blue
	colors.put(366, color::RGBA(209, 131, 4, 255));	  // Earth Orange
	colors.put(373, color::RGBA(124, 92, 137, 255));  // Sand Violet
	colors.put(375, color::RGBA(175, 181, 199, 255)); // Rubber Light Grey
	colors.put(378, color::RGBA(163, 162, 164, 255)); // Sand Green 2
	colors.put(379, color::RGBA(106, 121, 68, 255));  // Sand Yellow
	colors.put(462, color::RGBA(254, 138, 24, 255));  // Medium Orange
	colors.put(484, color::RGBA(170, 51, 21, 255));	  // Dark Orange
	colors.put(503, color::RGBA(199, 193, 183, 255)); // Very Light Grey
}

void LDrawFormat::parseColorMeta(const char *line, ColorMap &colors) {
	// format: 0 !COLOUR name CODE id VALUE #rrggbb EDGE edgeid [ALPHA a]
	const char *codeStr = strstr(line, "CODE ");
	const char *valueStr = strstr(line, "VALUE #");
	if (!codeStr || !valueStr) {
		return;
	}

	int code = atoi(codeStr + 5);
	const char *hex = valueStr + 7;
	if (strlen(hex) < 6) {
		return;
	}

	char hexBuf[7];
	memcpy(hexBuf, hex, 6);
	hexBuf[6] = '\0';
	unsigned int hexVal = (unsigned int)strtoul(hexBuf, nullptr, 16);
	uint8_t r = (uint8_t)((hexVal >> 16) & 0xFF);
	uint8_t g = (uint8_t)((hexVal >> 8) & 0xFF);
	uint8_t b = (uint8_t)(hexVal & 0xFF);
	uint8_t a = 255;

	const char *alphaStr = strstr(line, "ALPHA ");
	if (alphaStr) {
		a = (uint8_t)atoi(alphaStr + 6);
	}

	colors.put(code, color::RGBA(r, g, b, a));
}

void LDrawFormat::parseLDConfig(io::CachingArchive &archive, ColorMap &colors) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive.findStream("LDConfig.ldr"));
	if (!stream) {
		Log::debug("No ldconfig.ldr found in archive");
		return;
	}
	char line[512];
	while (stream->readLine(sizeof(line), line)) {
		if (strncmp(line, "0 !COLOUR ", 10) == 0) {
			parseColorMeta(line, colors);
		}
	}
	Log::debug("Parsed LDraw color config");
}

color::RGBA LDrawFormat::lookupColor(const ColorMap &colors, int colorCode) {
	color::RGBA rgba;
	if (colors.get(colorCode, rgba)) {
		return rgba;
	}
	// try to parse direct color codes (0x2RRGGBB format)
	if (colorCode >= 0x2000000 && colorCode <= 0x2FFFFFF) {
		uint8_t r = (uint8_t)((colorCode >> 16) & 0xFF);
		uint8_t g = (uint8_t)((colorCode >> 8) & 0xFF);
		uint8_t b = (uint8_t)(colorCode & 0xFF);
		return color::RGBA(r, g, b, 255);
	}
	Log::debug("Unknown LDraw color code %i, using grey", colorCode);
	return color::RGBA(127, 127, 127, 255);
}

static size_t countGeometryTris(const char *const *lines, size_t lineCount) {
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

void LDrawFormat::parseLine(const char *line, Mesh &mesh, ColorMap &colors, core::DynamicArray<SubFileRef> &subFiles,
							core::String &name, core::String &author) {
	// skip leading whitespace
	const char *ptr = line;
	while (*ptr == ' ' || *ptr == '\t') {
		++ptr;
	}
	if (*ptr == '\0' || *ptr == '\r' || *ptr == '\n') {
		return;
	}

	int lineType = *ptr - '0';
	if (lineType < 0 || lineType > 5) {
		return;
	}
	++ptr;
	while (*ptr == ' ' || *ptr == '\t') {
		++ptr;
	}

	if (lineType == 0) {
		// meta command or comment
		if (strncmp(ptr, "!COLOUR ", 8) == 0) {
			parseColorMeta(line, colors);
		} else if (strncmp(ptr, "Name: ", 6) == 0) {
			name = core::string::trim(core::String(ptr + 6));
		} else if (strncmp(ptr, "Author: ", 8) == 0) {
			author = core::string::trim(core::String(ptr + 8));
		}
		return;
	}

	if (lineType == 1) {
		// sub-file reference: 1 color x y z a b c d e f g h i filename
		SubFileRef ref;
		ref.colorCode = atoi(ptr);
		while (*ptr && *ptr != ' ')
			++ptr;
		while (*ptr == ' ')
			++ptr;

		const char *numStart = ptr;
		float values[12];
		for (int i = 0; i < 12; ++i) {
			if (!core::string::parseReal(&numStart, &values[i])) {
				Log::warn("Failed to parse sub-file reference values");
				return;
			}
		}
		ref.pos = glm::vec3(values[0], values[1], values[2]);
		ref.transform = glm::mat3(values[3], values[6], values[9], values[4], values[7], values[10], values[5],
								  values[8], values[11]);
		// skip to filename (after the 12 float values)
		while (*numStart == ' ' || *numStart == '\t')
			++numStart;
		ref.filename = core::string::trim(core::String(numStart));
		// remove trailing whitespace/newlines
		while (!ref.filename.empty() &&
			   (ref.filename.last() == '\r' || ref.filename.last() == '\n' || ref.filename.last() == ' ')) {
			ref.filename.erase(ref.filename.size() - 1, 1);
		}
		subFiles.push_back(core::move(ref));
		return;
	}

	if (lineType == 2) {
		// line - skip, not geometry we can voxelize
		return;
	}

	if (lineType == 3) {
		// triangle: 3 color x1 y1 z1 x2 y2 z2 x3 y3 z3
		int colorCode = atoi(ptr);
		while (*ptr && *ptr != ' ')
			++ptr;
		while (*ptr == ' ')
			++ptr;

		float vals[9];
		const char *numPtr = ptr;
		for (int i = 0; i < 9; ++i) {
			if (!core::string::parseReal(&numPtr, &vals[i])) {
				Log::warn("Failed to parse triangle values");
				return;
			}
		}

		color::RGBA rgba = lookupColor(colors, colorCode);
		for (int i = 0; i < 3; ++i) {
			MeshVertex vert;
			// LDraw: -Y is up, convert to Y up
			vert.pos = glm::vec3(vals[i * 3 + 0], -vals[i * 3 + 1], vals[i * 3 + 2]);
			vert.color = rgba;
			mesh.indices.push_back(mesh.vertices.size());
			mesh.vertices.emplace_back(core::move(vert));
		}
		return;
	}

	if (lineType == 4) {
		// quad: 4 color x1 y1 z1 x2 y2 z2 x3 y3 z3 x4 y4 z4
		int colorCode = atoi(ptr);
		while (*ptr && *ptr != ' ')
			++ptr;
		while (*ptr == ' ')
			++ptr;

		float vals[12];
		const char *numPtr = ptr;
		for (int i = 0; i < 12; ++i) {
			if (!core::string::parseReal(&numPtr, &vals[i])) {
				Log::warn("Failed to parse quad values");
				return;
			}
		}

		color::RGBA rgba = lookupColor(colors, colorCode);
		glm::vec3 verts[4];
		for (int i = 0; i < 4; ++i) {
			verts[i] = glm::vec3(vals[i * 3 + 0], -vals[i * 3 + 1], vals[i * 3 + 2]);
		}

		// split quad into two triangles: 0-1-2 and 0-2-3
		const int triIndices[] = {0, 1, 2, 0, 2, 3};
		for (int i = 0; i < 6; ++i) {
			MeshVertex vert;
			vert.pos = verts[triIndices[i]];
			vert.color = rgba;
			mesh.indices.push_back(mesh.vertices.size());
			mesh.vertices.emplace_back(core::move(vert));
		}
		return;
	}

	// lineType == 5: optional line - skip
}

bool LDrawFormat::resolveSubFile(io::CachingArchive &archive, const SubFileRef &ref, const ColorMap &colors,
								 Mesh &outMesh, int depth) const {
	static const int MaxDepth = 16;
	if (depth >= MaxDepth) {
		Log::warn("Maximum sub-file recursion depth reached for %s", ref.filename.c_str());
		return false;
	}
	if (stopExecution()) {
		return false;
	}

	core::ScopedPtr<io::SeekableReadStream> stream(archive.findStream(ref.filename));
	if (!stream) {
		Log::warn("Could not resolve sub-file reference: %s", ref.filename.c_str());
		return false;
	}
	Log::debug("Resolved sub-file: %s (depth %i)", ref.filename.c_str(), depth);

	// collect all lines first so we can pre-reserve geometry buffers
	core::DynamicArray<core::String> subLines;
	char line[512];
	while (stream->readLine(sizeof(line), line)) {
		subLines.push_back(core::String(line));
	}

	// build a pointer array for the counting helper
	core::DynamicArray<const char *> linePtrs;
	linePtrs.reserve(subLines.size());
	for (const core::String &l : subLines) {
		linePtrs.push_back(l.c_str());
	}
	const size_t estimatedTris = countGeometryTris(linePtrs.data(), linePtrs.size());

	// parse the sub-file
	ColorMap subColors = colors;
	core::DynamicArray<SubFileRef> subRefs;
	core::String subName;
	core::String subAuthor;

	{
		Mesh mesh;
		mesh.reserveAdditionalTris(estimatedTris);
		for (const core::String &l : subLines) {
			parseLine(l.c_str(), mesh, subColors, subRefs, subName, subAuthor);
		}

		// transform sub-mesh vertices and accumulate into output mesh
		const color::RGBA refColor = lookupColor(colors, ref.colorCode);
		outMesh.reserveAdditionalTris(mesh.vertices.size() / 3);
		for (size_t i = 0; i < mesh.vertices.size(); ++i) {
			MeshVertex vert = mesh.vertices[i];
			// vertices were parsed with -Y flip, undo it, apply LDraw transform, then re-flip
			vert.pos.y = -vert.pos.y;
			vert.pos = ref.transform * vert.pos + ref.pos;
			vert.pos.y = -vert.pos.y;
			// inherit color from parent if color 16 was used
			if (ref.colorCode != 16 && vert.color == lookupColor(colors, 16)) {
				vert.color = refColor;
			}
			outMesh.indices.push_back(outMesh.vertices.size());
			outMesh.vertices.emplace_back(core::move(vert));
		}
	}

	// recursively resolve sub-references, accumulating into the same output mesh
	for (const SubFileRef &subRef : subRefs) {
		SubFileRef transformedRef = subRef;
		// apply current transform to sub-reference
		transformedRef.pos = ref.pos + ref.transform * subRef.pos;
		transformedRef.transform = ref.transform * subRef.transform;
		// inherit color if sub-ref uses color 16 (main color)
		if (subRef.colorCode == 16) {
			transformedRef.colorCode = ref.colorCode;
		}
		resolveSubFile(archive, transformedRef, subColors, outMesh, depth + 1);
	}

	return true;
}

bool LDrawFormat::parseStream(io::SeekableReadStream &stream, Mesh &mesh, ColorMap &colors,
							  core::DynamicArray<SubFileRef> &subFiles, core::String &name,
							  core::String &author) const {
	char line[512];
	while (stream.readLine(sizeof(line), line)) {
		parseLine(line, mesh, colors, subFiles, name, author);
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

	io::CachingArchive cachedArchive(archive);

	// register LDraw library search directories
	const core::String ldrawDir = core::getVar(cfg::VoxformatLDrawDir)->strVal();
	Log::debug("LDraw library path: '%s'", ldrawDir.c_str());
	if (!ldrawDir.empty()) {
		const char *ldrawFilter = "*.dat,*.ldr";
		cachedArchive.registerSearchDir(ldrawDir, ldrawFilter);

		const char *searchDirs[] = {"parts", "p", "parts/s", "models"};
		for (const char *dir : searchDirs) {
			cachedArchive.registerSearchDir(core::string::path(ldrawDir, dir), ldrawFilter);
		}
	}

	ColorMap colors;
	initColors(colors);
	parseLDConfig(cachedArchive, colors);

	// check if this is an MPD file by scanning for "0 FILE" directives
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
			core::DynamicArray<SubFileRef> subFiles;
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
				parseLine(l.c_str(), mesh, colors, subFiles, name, author);
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

			for (const SubFileRef &ref : subFiles) {
				Mesh brickMesh;
				if (resolveSubFile(cachedArchive, ref, colors, brickMesh, 0) && !brickMesh.vertices.empty()) {
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

	// regular LDR file
	core::DynamicArray<SubFileRef> subFiles;
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
		parseLine(l.c_str(), mesh, colors, subFiles, name, author);
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

	for (const SubFileRef &ref : subFiles) {
		Mesh brickMesh;
		if (resolveSubFile(cachedArchive, ref, colors, brickMesh, 0) && !brickMesh.vertices.empty()) {
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
