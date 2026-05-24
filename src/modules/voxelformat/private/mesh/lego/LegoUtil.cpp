/**
 * @file
 */

#include "LegoUtil.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/CompositeArchive.h"
#include "io/FilesystemArchive.h"
#include <SDL_stdinc.h>

namespace voxelformat {

namespace legoutil {

void initColors(ColorMap &colors) {
	colors.put(0, color::RGBA(27, 42, 52, 255));
	colors.put(1, color::RGBA(0, 85, 191, 255));
	colors.put(2, color::RGBA(35, 120, 65, 255));
	colors.put(3, color::RGBA(0, 131, 143, 255));
	colors.put(4, color::RGBA(196, 40, 27, 255));
	colors.put(5, color::RGBA(205, 98, 152, 255));
	colors.put(6, color::RGBA(98, 71, 50, 255));
	colors.put(7, color::RGBA(156, 163, 168, 255));
	colors.put(8, color::RGBA(109, 110, 92, 255));
	colors.put(9, color::RGBA(180, 210, 227, 255));
	colors.put(10, color::RGBA(75, 159, 74, 255));
	colors.put(11, color::RGBA(85, 165, 175, 255));
	colors.put(12, color::RGBA(242, 112, 94, 255));
	colors.put(13, color::RGBA(252, 151, 172, 255));
	colors.put(14, color::RGBA(245, 205, 47, 255));
	colors.put(15, color::RGBA(255, 255, 255, 255));
	colors.put(16, color::RGBA(127, 127, 127, 255));
	colors.put(17, color::RGBA(163, 228, 215, 255));
	colors.put(18, color::RGBA(253, 234, 140, 255));
	colors.put(19, color::RGBA(232, 207, 161, 255));
	colors.put(20, color::RGBA(215, 196, 230, 255));
	colors.put(22, color::RGBA(129, 0, 123, 255));
	colors.put(23, color::RGBA(0, 51, 178, 255));
	colors.put(24, color::RGBA(0, 0, 0, 255));
	colors.put(25, color::RGBA(224, 152, 100, 255));
	colors.put(26, color::RGBA(218, 133, 218, 255));
	colors.put(27, color::RGBA(163, 228, 40, 255));
	colors.put(28, color::RGBA(197, 157, 115, 255));
	colors.put(29, color::RGBA(228, 173, 200, 255));
	colors.put(33, color::RGBA(0, 30, 160, 128));
	colors.put(34, color::RGBA(86, 229, 100, 128));
	colors.put(36, color::RGBA(201, 26, 9, 128));
	colors.put(40, color::RGBA(99, 95, 82, 128));
	colors.put(41, color::RGBA(174, 233, 239, 128));
	colors.put(42, color::RGBA(192, 255, 0, 128));
	colors.put(43, color::RGBA(100, 176, 227, 128));
	colors.put(44, color::RGBA(150, 112, 159, 128));
	colors.put(46, color::RGBA(247, 241, 141, 128));
	colors.put(47, color::RGBA(252, 252, 252, 128));
	colors.put(52, color::RGBA(165, 105, 200, 128));
	colors.put(57, color::RGBA(243, 207, 155, 128));
	colors.put(68, color::RGBA(243, 183, 40, 255));
	colors.put(69, color::RGBA(211, 62, 159, 255));
	colors.put(70, color::RGBA(105, 64, 39, 255));
	colors.put(71, color::RGBA(163, 162, 164, 255));
	colors.put(72, color::RGBA(99, 95, 97, 255));
	colors.put(73, color::RGBA(110, 153, 201, 255));
	colors.put(74, color::RGBA(161, 196, 139, 255));
	colors.put(77, color::RGBA(220, 144, 149, 255));
	colors.put(78, color::RGBA(249, 197, 131, 255));
	colors.put(84, color::RGBA(204, 112, 42, 255));
	colors.put(85, color::RGBA(107, 50, 123, 255));
	colors.put(86, color::RGBA(125, 91, 56, 255));
	colors.put(89, color::RGBA(76, 145, 198, 255));
	colors.put(92, color::RGBA(204, 142, 104, 255));
	colors.put(100, color::RGBA(238, 196, 182, 255));
	colors.put(110, color::RGBA(67, 84, 163, 255));
	colors.put(112, color::RGBA(72, 97, 172, 255));
	colors.put(115, color::RGBA(199, 210, 60, 255));
	colors.put(118, color::RGBA(183, 215, 213, 255));
	colors.put(120, color::RGBA(216, 221, 86, 255));
	colors.put(125, color::RGBA(252, 172, 0, 255));
	colors.put(151, color::RGBA(108, 110, 104, 255));
	colors.put(191, color::RGBA(248, 187, 61, 255));
	colors.put(212, color::RGBA(155, 178, 239, 255));
	colors.put(216, color::RGBA(143, 76, 42, 255));
	colors.put(226, color::RGBA(253, 234, 140, 255));
	colors.put(232, color::RGBA(125, 187, 221, 255));
	colors.put(256, color::RGBA(33, 33, 33, 255));
	colors.put(272, color::RGBA(32, 50, 176, 255));
	colors.put(288, color::RGBA(39, 70, 44, 255));
	colors.put(308, color::RGBA(53, 33, 0, 255));
	colors.put(320, color::RGBA(120, 0, 28, 255));
	colors.put(334, color::RGBA(189, 125, 8, 255));
	colors.put(335, color::RGBA(224, 224, 224, 255));
	colors.put(366, color::RGBA(209, 131, 4, 255));
	colors.put(373, color::RGBA(124, 92, 137, 255));
	colors.put(375, color::RGBA(175, 181, 199, 255));
	colors.put(378, color::RGBA(163, 162, 164, 255));
	colors.put(379, color::RGBA(106, 121, 68, 255));
	colors.put(462, color::RGBA(254, 138, 24, 255));
	colors.put(484, color::RGBA(170, 51, 21, 255));
	colors.put(503, color::RGBA(199, 193, 183, 255));
}

void parseColorMeta(const char *line, ColorMap &colors) {
	const char *codeStr = strstr(line, "CODE ");
	const char *valueStr = strstr(line, "VALUE #");
	if (!codeStr || !valueStr) {
		return;
	}

	const int code = atoi(codeStr + 5);
	const char *hex = valueStr + 7;
	if (strlen(hex) < 6) {
		return;
	}

	char hexBuf[7];
	memcpy(hexBuf, hex, 6);
	hexBuf[6] = '\0';
	const unsigned int hexVal = (unsigned int)strtoul(hexBuf, nullptr, 16);
	const uint8_t r = (uint8_t)((hexVal >> 16) & 0xFF);
	const uint8_t g = (uint8_t)((hexVal >> 8) & 0xFF);
	const uint8_t b = (uint8_t)(hexVal & 0xFF);
	uint8_t a = 255;

	const char *alphaStr = strstr(line, "ALPHA ");
	if (alphaStr) {
		a = (uint8_t)atoi(alphaStr + 6);
	}

	colors.put(code, color::RGBA(r, g, b, a));
}

void parseLDConfig(io::CachingArchive &archive, ColorMap &colors) {
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

void parseLegoIdMap(io::SeekableReadStream &stream, ColorMap &colors, core::DynamicMap<int, int> &legoIdToLdrawCode) {
	char line[512];
	int pendingLegoId = -1;
	while (stream.readLine(sizeof(line), line)) {
		const char *ptr = line;
		while (*ptr == ' ' || *ptr == '\t') {
			++ptr;
		}
		// LDraw comment lines start with '0' followed by the comment content
		if (*ptr == '0') {
			++ptr;
			while (*ptr == ' ' || *ptr == '\t') {
				++ptr;
			}
		}
		if (strncmp(ptr, "// LEGOID", 9) == 0) {
			const char *idStr = ptr + 9;
			while (*idStr == ' ') {
				++idStr;
			}
			pendingLegoId = SDL_atoi(idStr);
			continue;
		}
		if (strncmp(ptr, "!COLOUR ", 8) == 0 || strncmp(line, "0 !COLOUR ", 10) == 0) {
			parseColorMeta(line, colors);
			if (pendingLegoId >= 0) {
				const char *codeStr = strstr(line, "CODE ");
				if (codeStr != nullptr) {
					const int ldrawCode = SDL_atoi(codeStr + 5);
					legoIdToLdrawCode.put(pendingLegoId, ldrawCode);
				}
				pendingLegoId = -1;
			}
		}
	}
}

color::RGBA lookupColor(const ColorMap &colors, int colorCode) {
	color::RGBA rgba;
	if (colors.get(colorCode, rgba)) {
		return rgba;
	}
	if (colorCode >= 0x2000000 && colorCode <= 0x2FFFFFF) {
		const uint8_t r = (uint8_t)((colorCode >> 16) & 0xFF);
		const uint8_t g = (uint8_t)((colorCode >> 8) & 0xFF);
		const uint8_t b = (uint8_t)(colorCode & 0xFF);
		return color::RGBA(r, g, b, 255);
	}
	Log::debug("Unknown LDraw color code %i, using grey", colorCode);
	return color::RGBA(127, 127, 127, 255);
}

int lookupLdrawColor(const core::DynamicMap<int, int> &legoIdToLdrawCode, int legoMaterialId) {
	int ldrawCode = legoMaterialId;
	if (legoIdToLdrawCode.get(legoMaterialId, ldrawCode)) {
		return ldrawCode;
	}
	return legoMaterialId;
}

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

void parseLine(const char *line, Mesh &mesh, ColorMap &colors, core::DynamicArray<SubFileRef> &subFiles,
			   core::String &name, core::String &author) {
	const char *ptr = line;
	while (*ptr == ' ' || *ptr == '\t') {
		++ptr;
	}
	if (*ptr == '\0' || *ptr == '\r' || *ptr == '\n') {
		return;
	}

	const int lineType = *ptr - '0';
	if (lineType < 0 || lineType > 5) {
		return;
	}
	++ptr;
	while (*ptr == ' ' || *ptr == '\t') {
		++ptr;
	}

	if (lineType == 0) {
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
		SubFileRef ref;
		ref.colorCode = atoi(ptr);
		while (*ptr && *ptr != ' ') {
			++ptr;
		}
		while (*ptr == ' ') {
			++ptr;
		}

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
		while (*numStart == ' ' || *numStart == '\t') {
			++numStart;
		}
		ref.filename = core::string::trim(core::String(numStart));
		while (!ref.filename.empty() &&
			   (ref.filename.last() == '\r' || ref.filename.last() == '\n' || ref.filename.last() == ' ')) {
			ref.filename.erase(ref.filename.size() - 1, 1);
		}
		subFiles.push_back(core::move(ref));
		return;
	}

	if (lineType == 2) {
		return;
	}

	if (lineType == 3) {
		const int colorCode = atoi(ptr);
		while (*ptr && *ptr != ' ') {
			++ptr;
		}
		while (*ptr == ' ') {
			++ptr;
		}

		float vals[9];
		const char *numPtr = ptr;
		for (int i = 0; i < 9; ++i) {
			if (!core::string::parseReal(&numPtr, &vals[i])) {
				Log::warn("Failed to parse triangle values");
				return;
			}
		}

		const color::RGBA rgba = lookupColor(colors, colorCode);
		for (int i = 0; i < 3; ++i) {
			MeshVertex vert;
			vert.pos = glm::vec3(vals[i * 3 + 0], -vals[i * 3 + 1], -vals[i * 3 + 2]);
			vert.color = rgba;
			mesh.indices.push_back(mesh.vertices.size());
			mesh.vertices.emplace_back(core::move(vert));
		}
		// Reverse winding order to compensate for the Z negation (odd reflection)
		const size_t last = mesh.indices.size();
		if (last >= 3) {
			const auto tmp = mesh.indices[last - 1];
			mesh.indices[last - 1] = mesh.indices[last - 2];
			mesh.indices[last - 2] = tmp;
		}
		return;
	}

	if (lineType == 4) {
		const int colorCode = atoi(ptr);
		while (*ptr && *ptr != ' ') {
			++ptr;
		}
		while (*ptr == ' ') {
			++ptr;
		}

		float vals[12];
		const char *numPtr = ptr;
		for (int i = 0; i < 12; ++i) {
			if (!core::string::parseReal(&numPtr, &vals[i])) {
				Log::warn("Failed to parse quad values");
				return;
			}
		}

		const color::RGBA rgba = lookupColor(colors, colorCode);
		glm::vec3 verts[4];
		for (int i = 0; i < 4; ++i) {
			verts[i] = glm::vec3(vals[i * 3 + 0], -vals[i * 3 + 1], -vals[i * 3 + 2]);
		}

		// Reverse winding order to compensate for the Z negation (odd reflection)
		const int triIndices[] = {0, 2, 1, 0, 3, 2};
		for (int i = 0; i < 6; ++i) {
			MeshVertex vert;
			vert.pos = verts[triIndices[i]];
			vert.color = rgba;
			mesh.indices.push_back(mesh.vertices.size());
			mesh.vertices.emplace_back(core::move(vert));
		}
	}
}

bool resolveSubFile(io::CachingArchive &archive, const SubFileRef &ref, const ColorMap &colors, Mesh &outMesh,
					int depth) {
	static const int MaxDepth = 16;
	if (depth >= MaxDepth) {
		Log::warn("Maximum sub-file recursion depth reached for %s", ref.filename.c_str());
		return false;
	}
	if (app::App::getInstance()->shouldQuit()) {
		return false;
	}

	core::ScopedPtr<io::SeekableReadStream> stream(archive.findStream(ref.filename));
	if (!stream) {
		Log::warn("Could not resolve sub-file reference: %s", ref.filename.c_str());
		return false;
	}
	Log::debug("Resolved sub-file: %s (depth %i)", ref.filename.c_str(), depth);

	core::DynamicArray<core::String> subLines;
	char line[512];
	while (stream->readLine(sizeof(line), line)) {
		subLines.push_back(core::String(line));
	}

	core::DynamicArray<const char *> linePtrs;
	linePtrs.reserve(subLines.size());
	for (const core::String &l : subLines) {
		linePtrs.push_back(l.c_str());
	}
	const size_t estimatedTris = countGeometryTris(linePtrs.data(), linePtrs.size());

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

		const color::RGBA refColor = lookupColor(colors, ref.colorCode);
		outMesh.reserveAdditionalTris(mesh.vertices.size() / 3);
		for (size_t i = 0; i < mesh.vertices.size(); ++i) {
			MeshVertex vert = mesh.vertices[i];
			vert.pos.y = -vert.pos.y;
			vert.pos.z = -vert.pos.z;
			vert.pos = ref.transform * vert.pos + ref.pos;
			vert.pos.y = -vert.pos.y;
			vert.pos.z = -vert.pos.z;
			if (ref.colorCode != 16 && vert.color == lookupColor(colors, 16)) {
				vert.color = refColor;
			}
			outMesh.indices.push_back(outMesh.vertices.size());
			outMesh.vertices.emplace_back(core::move(vert));
		}
	}

	for (const SubFileRef &subRef : subRefs) {
		SubFileRef transformedRef = subRef;
		transformedRef.pos = ref.pos + ref.transform * subRef.pos;
		transformedRef.transform = ref.transform * subRef.transform;
		if (subRef.colorCode == 16) {
			transformedRef.colorCode = ref.colorCode;
		}
		resolveSubFile(archive, transformedRef, subColors, outMesh, depth + 1);
	}

	return true;
}

io::ArchivePtr openLookupArchive(const io::ArchivePtr &archive) {
	const core::String ldrawDir = core::getVar(cfg::VoxformatLDrawDir)->strVal();
	if (ldrawDir.empty() || archive->exists(ldrawDir)) {
		return archive;
	}
	const io::ArchivePtr fsArchive = io::openFilesystemArchive(io::filesystem());
	if (!fsArchive) {
		return archive;
	}
	core::SharedPtr<io::CompositeArchive> composite = core::make_shared<io::CompositeArchive>();
	composite->add(archive);
	composite->add(fsArchive);
	return composite;
}

void registerLdrawSearchPaths(io::CachingArchive &cachedArchive) {
	const core::String ldrawDir = core::getVar(cfg::VoxformatLDrawDir)->strVal();
	const char *ldrawFilter = "*.dat,*.ldr";
	if (!ldrawDir.empty()) {
		cachedArchive.registerSearchDir(ldrawDir, ldrawFilter);
		return;
	}
	cachedArchive.registerSearchDir("parts", ldrawFilter);
	cachedArchive.registerSearchDir("p", ldrawFilter);
	cachedArchive.registerSearchDir("models", ldrawFilter);
}

} // namespace legoutil

} // namespace voxelformat
