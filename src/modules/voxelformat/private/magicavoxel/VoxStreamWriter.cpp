/**
 * @file
 * @brief Streaming VOX writer implementation.
 */

#include "VoxStreamWriter.h"
#include "MagicaVoxel.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"
#include "voxelformat/external/ogt_vox.h"

namespace voxelformat {

static constexpr uint32_t VOX_MAGIC = 0x20584F56; // "VOX "
static constexpr uint32_t VOX_VERSION = 150;

// Write chunk header with placeholder content size. Returns position of content_size field for patching.
static int64_t writeChunkStart(io::SeekableWriteStream *s, const char *id) {
	s->write(id, 4);
	const int64_t sizePos = s->pos();
	s->writeUInt32(0); // placeholder content size
	s->writeUInt32(0); // child size (always 0 for leaf chunks)
	return sizePos;
}

// Patch the content size of a chunk that was started with writeChunkStart.
static void writeChunkEnd(io::SeekableWriteStream *s, int64_t sizePos) {
	const int64_t endPos = s->pos();
	const uint32_t contentSize = (uint32_t)(endPos - sizePos - 8); // subtract size(4) + childSize(4)
	s->seek(sizePos);
	s->writeUInt32(contentSize);
	s->seek(endPos);
}

// Write a single dict entry (key + value) to the stream.
static void writeDictEntry(io::SeekableWriteStream *s, const char *key, const core::String &value) {
	const uint32_t keyLen = (uint32_t)SDL_strlen(key);
	s->writeUInt32(keyLen);
	s->write(key, keyLen);
	s->writeUInt32((uint32_t)value.size());
	s->write(value.c_str(), value.size());
}

// Write an empty dict (0 entries).
static void writeEmptyDict(io::SeekableWriteStream *s) {
	s->writeUInt32(0);
}

/**
 * @brief Write a single model's voxels as SIZE + XYZI chunks directly to the stream.
 * Only non-air voxels are written (sparse XYZI format). No dense array is allocated.
 */
static void writeModelChunks(io::SeekableWriteStream *s, const MVModelRef &model) {
	const voxel::Region &region = model.region;
	const glm::ivec3 dims = region.getDimensionsInVoxels();
	// SIZE chunk: model dimensions (flip y/z for MagicaVoxel)
	const int64_t sizeSizePos = writeChunkStart(s, "SIZE");
	s->writeUInt32(dims.x);
	s->writeUInt32(dims.z); // MV Y = vengi Z
	s->writeUInt32(dims.y); // MV Z = vengi Y
	writeChunkEnd(s, sizeSizePos);

	// XYZI chunk: sparse voxel data
	const int64_t xyziSizePos = writeChunkStart(s, "XYZI");
	const int64_t countPos = s->pos();
	s->writeUInt32(0); // placeholder voxel count

	const glm::ivec3 lo = region.getLowerCorner();
	const glm::ivec3 hi = region.getUpperCorner();
	uint32_t numVoxels = 0;
	// Iterate in MV storage order: k(MV Z = vengi Y), j(MV Y = vengi Z), i(MV X = -vengi X)
	for (int y = lo.y; y <= hi.y; ++y) {
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int x = hi.x; x >= lo.x; --x) {
				const voxel::Voxel &voxel = model.volume->voxel(x, y, z);
				if (voxel.getMaterial() == voxel::VoxelType::Air) {
					continue;
				}
				const uint8_t color = model.colorRemap[voxel.getColor()];
				if (color == 0) {
					continue;
				}
				s->writeUInt8((uint8_t)(hi.x - x));
				s->writeUInt8((uint8_t)(z - lo.z));
				s->writeUInt8((uint8_t)(y - lo.y));
				s->writeUInt8(color);
				++numVoxels;
			}
		}
	}

	// Patch voxel count
	const int64_t endPos = s->pos();
	s->seek(countPos);
	s->writeUInt32(numVoxels);
	s->seek(endPos);
	writeChunkEnd(s, xyziSizePos);
}

static core::String transformString(const ogt_vox_transform &t) {
	core::String s;
	s.append(core::string::toString((int)t.m30));
	s.append(" ");
	s.append(core::string::toString((int)t.m31));
	s.append(" ");
	s.append(core::string::toString((int)t.m32));
	return s;
}

// Encode rotation matrix into MagicaVoxel packed rotation byte.
// OGT stores column-major: row0 = (m00, m10, m20), row1 = (m01, m11, m21), row2 = (m02, m12, m22)
static uint8_t encodeRotation(const ogt_vox_transform &t) {
	// Extract rows (column-major to row-major swizzle, matching OGT convention)
	float row[3][3] = {{t.m00, t.m10, t.m20}, {t.m01, t.m11, t.m21}, {t.m02, t.m12, t.m22}};
	int idx0 = 0;
	int idx1 = 1;
	for (int c = 0; c < 3; ++c) {
		if (row[0][c] == 1.0f || row[0][c] == -1.0f) {
			idx0 = c;
		}
		if (row[1][c] == 1.0f || row[1][c] == -1.0f) {
			idx1 = c;
		}
	}
	uint8_t packed = (uint8_t)((idx0) | (idx1 << 2));
	if (row[0][idx0] < 0.0f) {
		packed |= (1 << 4);
	}
	if (row[1][idx1] < 0.0f) {
		packed |= (1 << 5);
	}
	const int idx2 = 3 - idx0 - idx1;
	if (row[2][idx2] < 0.0f) {
		packed |= (1 << 6);
	}
	return packed;
}

// Write an nTRN chunk with inline dict + keyframe dict (seek-and-patch for sizes)
static void writeTransformNode(io::SeekableWriteStream *s, uint32_t nodeId, uint32_t childId, uint32_t layerIdx,
							   const char *name, bool hidden, uint32_t numKeyframes,
							   const ogt_vox_keyframe_transform *keyframes, const ogt_vox_transform *groupTransform) {
	const int64_t sizePos = writeChunkStart(s, "nTRN");
	s->writeUInt32(nodeId);

	// Node dict
	uint32_t numDictEntries = 0;
	if (name != nullptr && name[0] != '\0') {
		++numDictEntries;
	}
	if (hidden) {
		++numDictEntries;
	}
	s->writeUInt32(numDictEntries);
	if (name != nullptr && name[0] != '\0') {
		writeDictEntry(s, "_name", name);
	}
	if (hidden) {
		writeDictEntry(s, "_hidden", "1");
	}

	s->writeUInt32(childId);
	s->writeUInt32(UINT32_MAX); // reserved
	s->writeUInt32(layerIdx);
	s->writeUInt32(numKeyframes);

	for (uint32_t k = 0; k < numKeyframes; ++k) {
		// Frame dict - always _r and _t, plus _f when there are keyframes
		const ogt_vox_transform &t = keyframes ? keyframes[k].transform : *groupTransform;
		if (keyframes != nullptr) {
			s->writeUInt32(3); // _r, _t, _f
			writeDictEntry(s, "_r", core::string::toString((int)encodeRotation(t)));
			writeDictEntry(s, "_t", transformString(t));
			writeDictEntry(s, "_f", core::string::toString(keyframes[k].frame_index));
		} else {
			s->writeUInt32(2); // _r, _t
			writeDictEntry(s, "_r", core::string::toString((int)encodeRotation(t)));
			writeDictEntry(s, "_t", transformString(t));
		}
	}
	writeChunkEnd(s, sizePos);
}

bool saveGroupsStreamed(io::SeekableWriteStream *s, const MVSceneContext &ctx,
						const scenegraph::SceneGraph &sceneGraph) {
	const uint32_t numGroups = (uint32_t)ctx.groups.size();
	const uint32_t numInstances = (uint32_t)ctx.instances.size();

	// Write VOX header
	s->writeUInt32(VOX_MAGIC);
	s->writeUInt32(VOX_VERSION);

	// Write MAIN chunk header (child size patched at end)
	const int64_t mainSizePos = writeChunkStart(s, "MAIN");

	// Write each model as SIZE + XYZI chunks (streaming - one at a time, no dense array held)
	for (const MVModelRef &model : ctx.models) {
		writeModelChunks(s, model);
	}

	// Node ID layout (matches OGT convention)
	const uint32_t firstGroupTransformId = 0;
	const uint32_t firstGroupId = numGroups;
	const uint32_t firstShapeId = firstGroupId + numGroups;
	const uint32_t firstInstanceTransformId = firstShapeId + numInstances;

	// Write group transform nodes (nTRN)
	for (uint32_t g = 0; g < numGroups; ++g) {
		const ogt_vox_group &group = ctx.groups[g];
		writeTransformNode(s, firstGroupTransformId + g, firstGroupId + g, group.layer_index, group.name, group.hidden,
						   1, nullptr, &group.transform);
	}

	// Write group nodes (nGRP)
	for (uint32_t g = 0; g < numGroups; ++g) {
		const int64_t sizePos = writeChunkStart(s, "nGRP");
		s->writeUInt32(firstGroupId + g);
		// Dict
		if (ctx.groups[g].hidden) {
			s->writeUInt32(1);
			writeDictEntry(s, "_hidden", "1");
		} else {
			writeEmptyDict(s);
		}
		// Count and write child IDs
		core::Buffer<uint32_t> childIds;
		for (uint32_t cg = 0; cg < numGroups; ++cg) {
			if (ctx.groups[cg].parent_group_index == g) {
				childIds.push_back(firstGroupTransformId + cg);
			}
		}
		for (uint32_t ci = 0; ci < numInstances; ++ci) {
			if (ctx.instances[ci].group_index == g) {
				childIds.push_back(firstInstanceTransformId + ci);
			}
		}
		s->writeUInt32((uint32_t)childIds.size());
		for (uint32_t id : childIds) {
			s->writeUInt32(id);
		}
		writeChunkEnd(s, sizePos);
	}

	// Write shape nodes (nSHP)
	for (uint32_t i = 0; i < numInstances; ++i) {
		const ogt_vox_instance &inst = ctx.instances[i];
		const int64_t sizePos = writeChunkStart(s, "nSHP");
		s->writeUInt32(firstShapeId + i);
		// nSHP node dict - only _loop if model_anim has loop set
		if (inst.model_anim.loop) {
			s->writeUInt32(1);
			writeDictEntry(s, "_loop", "1");
		} else {
			writeEmptyDict(s);
		}
		if (inst.model_anim.num_keyframes > 0) {
			s->writeUInt32(inst.model_anim.num_keyframes);
			for (uint32_t k = 0; k < inst.model_anim.num_keyframes; ++k) {
				s->writeUInt32(inst.model_anim.keyframes[k].model_index);
				s->writeUInt32(1); // dict with _f key
				writeDictEntry(s, "_f", core::string::toString(inst.model_anim.keyframes[k].frame_index));
			}
		} else {
			s->writeUInt32(1); // num_models = 1
			s->writeUInt32(inst.model_index);
			writeEmptyDict(s); // model attributes dict
		}
		writeChunkEnd(s, sizePos);
	}

	// Write instance transform nodes (nTRN)
	for (uint32_t i = 0; i < numInstances; ++i) {
		const ogt_vox_instance &inst = ctx.instances[i];
		const uint32_t numKf = inst.transform_anim.num_keyframes > 0 ? inst.transform_anim.num_keyframes : 1;
		writeTransformNode(s, firstInstanceTransformId + i, firstShapeId + i, inst.layer_index, inst.name, inst.hidden,
						   numKf, inst.transform_anim.keyframes, nullptr);
	}

	// Write rCAM chunks
	for (uint32_t i = 0; i < (uint32_t)ctx.cameras.size(); ++i) {
		const ogt_vox_cam &cam = ctx.cameras[i];
		const int64_t sizePos = writeChunkStart(s, "rCAM");
		s->writeUInt32(cam.camera_id);
		s->writeUInt32(6); // 6 dict entries
		writeDictEntry(s, "_mode", cam.mode == ogt_cam_mode_perspective ? "pers" : "orth");
		core::String focusStr;
		focusStr.append(core::string::toString(cam.focus[0]));
		focusStr.append(" ");
		focusStr.append(core::string::toString(cam.focus[1]));
		focusStr.append(" ");
		focusStr.append(core::string::toString(cam.focus[2]));
		writeDictEntry(s, "_focus", focusStr);
		core::String angleStr;
		angleStr.append(core::string::toString(cam.angle[0]));
		angleStr.append(" ");
		angleStr.append(core::string::toString(cam.angle[1]));
		angleStr.append(" ");
		angleStr.append(core::string::toString(cam.angle[2]));
		writeDictEntry(s, "_angle", angleStr);
		writeDictEntry(s, "_radius", core::string::toString(cam.radius));
		writeDictEntry(s, "_fov", core::string::toString((int)cam.fov));
		writeDictEntry(s, "_frustum", core::string::toString(cam.frustum));
		writeChunkEnd(s, sizePos);
	}

	// Write RGBA palette - rotated by +1 to match VOX file format convention.
	// OGT reader rotates back on load (shifts by -1, sets color[0]=air).
	// Use the merged palette when available (streaming path with per-model color remap).
	const palette::Palette &palette = ctx.mergedPalette ? *ctx.mergedPalette : sceneGraph.firstPalette();
	{
		const int64_t sizePos = writeChunkStart(s, "RGBA");
		for (int i = 0; i < 256; ++i) {
			const int idx = (i + 1) & 255;
			const color::RGBA &c = palette.color(idx);
			s->writeUInt8(c.r);
			s->writeUInt8(c.g);
			s->writeUInt8(c.b);
			s->writeUInt8(c.a);
		}
		writeChunkEnd(s, sizePos);
	}

	// Write NOTE chunk (color names)
	if (palette.colorCount() > 0) {
		const int64_t sizePos = writeChunkStart(s, "NOTE");
		s->writeUInt32(palette.colorCount());
		for (int i = 0; i < palette.colorCount(); ++i) {
			const core::String &name = palette.colorName(i);
			s->writeUInt32((uint32_t)name.size());
			if (!name.empty()) {
				s->write(name.c_str(), name.size());
			}
		}
		writeChunkEnd(s, sizePos);
	}

	// Write MATL chunks
	for (int i = 0; i < palette.colorCount(); ++i) {
		const palette::Material &material = palette.material(i);
		const color::RGBA &rgba = palette.color(i);

		// Count material properties
		int numProps = 1; // _type always present
		if (material.has(palette::MaterialProperty::MaterialMetal)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialRoughness)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialSpecular)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialIndexOfRefraction)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialAttenuation)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialFlux)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialEmit)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialLowDynamicRange)) { ++numProps; }
		if (rgba.a < 255) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialDensity)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialSp)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialPhase)) { ++numProps; }
		if (material.has(palette::MaterialProperty::MaterialMedia)) { ++numProps; }

		if (numProps <= 1) {
			continue; // Only _type, skip
		}

		const int64_t sizePos = writeChunkStart(s, "MATL");
		s->writeUInt32((uint32_t)i); // 0-indexed, matches OGT convention

		s->writeUInt32(numProps);
		const char *typeStr = "_diffuse";
		if (material.type == palette::MaterialType::Metal) { typeStr = "_metal"; }
		else if (material.type == palette::MaterialType::Glass) { typeStr = "_glass"; }
		else if (material.type == palette::MaterialType::Emit) { typeStr = "_emit"; }
		else if (material.type == palette::MaterialType::Blend) { typeStr = "_blend"; }
		else if (material.type == palette::MaterialType::Media) { typeStr = "_media"; }
		writeDictEntry(s, "_type", typeStr);

		if (material.has(palette::MaterialProperty::MaterialMetal)) {
			writeDictEntry(s, "_metal", core::string::toString(material.value(palette::MaterialMetal)));
		}
		if (material.has(palette::MaterialProperty::MaterialRoughness)) {
			writeDictEntry(s, "_rough", core::string::toString(material.value(palette::MaterialRoughness)));
		}
		if (material.has(palette::MaterialProperty::MaterialSpecular)) {
			writeDictEntry(s, "_spec", core::string::toString(material.value(palette::MaterialSpecular)));
		}
		if (material.has(palette::MaterialProperty::MaterialIndexOfRefraction)) {
			writeDictEntry(s, "_ior", core::string::toString(material.value(palette::MaterialIndexOfRefraction)));
		}
		if (material.has(palette::MaterialProperty::MaterialAttenuation)) {
			writeDictEntry(s, "_att", core::string::toString(material.value(palette::MaterialAttenuation)));
		}
		if (material.has(palette::MaterialProperty::MaterialFlux)) {
			writeDictEntry(s, "_flux", core::string::toString(material.value(palette::MaterialFlux)));
		}
		if (material.has(palette::MaterialProperty::MaterialEmit)) {
			writeDictEntry(s, "_emit", core::string::toString(material.value(palette::MaterialEmit)));
		}
		if (material.has(palette::MaterialProperty::MaterialLowDynamicRange)) {
			writeDictEntry(s, "_ldr", core::string::toString(material.value(palette::MaterialLowDynamicRange)));
		}
		if (rgba.a < 255) {
			writeDictEntry(s, "_alpha", core::string::toString((float)rgba.a / 255.0f));
		}
		if (material.has(palette::MaterialProperty::MaterialDensity)) {
			writeDictEntry(s, "_d", core::string::toString(material.value(palette::MaterialDensity)));
		}
		if (material.has(palette::MaterialProperty::MaterialSp)) {
			writeDictEntry(s, "_sp", core::string::toString(material.value(palette::MaterialSp)));
		}
		if (material.has(palette::MaterialProperty::MaterialPhase)) {
			writeDictEntry(s, "_g", core::string::toString(material.value(palette::MaterialPhase)));
		}
		if (material.has(palette::MaterialProperty::MaterialMedia)) {
			writeDictEntry(s, "_media", core::string::toString(material.value(palette::MaterialMedia)));
		}
		writeChunkEnd(s, sizePos);
	}

	// Write LAYR chunks
	for (uint32_t i = 0; i < (uint32_t)ctx.layers.size(); ++i) {
		const ogt_vox_layer &layer = ctx.layers[i];
		const int64_t sizePos = writeChunkStart(s, "LAYR");
		s->writeUInt32(i);

		int numEntries = 1; // _color is always written
		if (layer.name != nullptr && layer.name[0] != '\0') { ++numEntries; }
		if (layer.hidden) { ++numEntries; }

		s->writeUInt32(numEntries);
		if (layer.name != nullptr && layer.name[0] != '\0') {
			writeDictEntry(s, "_name", layer.name);
		}
		if (layer.hidden) {
			writeDictEntry(s, "_hidden", "1");
		}
		core::String colorStr;
		colorStr.append(core::string::toString((int)layer.color.r));
		colorStr.append(" ");
		colorStr.append(core::string::toString((int)layer.color.g));
		colorStr.append(" ");
		colorStr.append(core::string::toString((int)layer.color.b));
		writeDictEntry(s, "_color", colorStr);

		s->writeUInt32(UINT32_MAX); // reserved
		writeChunkEnd(s, sizePos);
	}

	// Patch MAIN chunk child size
	const int64_t endPos = s->pos();
	const uint32_t mainChildSize = (uint32_t)(endPos - mainSizePos - 8);
	s->seek(mainSizePos + 4); // skip content_size, write child_size
	s->writeUInt32(mainChildSize);
	s->seek(endPos);

	// no debug dump

	return true;
}

} // namespace voxelformat
