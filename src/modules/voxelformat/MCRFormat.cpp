/**
 * @file
 */

#include "MCRFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeMerger.h"

#define MINIZ_NO_STDIO
// Note: we do not need miniz stdio functions so can define MINIZ_NO_STDIO in project to remove them
#include "core/miniz.h"
#include "external/enkimi.c"

namespace voxel {
namespace _priv {
static const uint32_t mcpalette[] = {
	0xff000000, 0xff7d7d7d, 0xff4cb376, 0xff436086, 0xff7a7a7a, 0xff4e7f9c, 0xff256647, 0xff535353, 0xffdcaf70,
	0xffdcaf70, 0xff135bcf, 0xff125ad4, 0xffa0d3db, 0xff7a7c7e, 0xff7c8b8f, 0xff7e8287, 0xff737373, 0xff315166,
	0xff31b245, 0xff54c3c2, 0xfff4f0da, 0xff867066, 0xff894326, 0xff838383, 0xff9fd3dc, 0xff324364, 0xff3634b4,
	0xff23c7f6, 0xff7c7c7c, 0xff77bf8e, 0xffdcdcdc, 0xff296595, 0xff194f7b, 0xff538ba5, 0xff5e96bd, 0xffdddddd,
	0xffe5e5e5, 0xff00ffff, 0xff0d00da, 0xff415778, 0xff0d0fe1, 0xff4eecf9, 0xffdbdbdb, 0xffa1a1a1, 0xffa6a6a6,
	0xff0630bc, 0xff0026af, 0xff39586b, 0xff658765, 0xff1d1214, 0xff00ffff, 0xff005fde, 0xff31271a, 0xff4e87a6,
	0xff2a74a4, 0xff0000ff, 0xff8f8c81, 0xffd5db61, 0xff2e5088, 0xff17593c, 0xff335682, 0xff676767, 0xff00b9ff,
	0xff5b9ab8, 0xff387394, 0xff345f79, 0xff5190b6, 0xff6a6a6a, 0xff5b9ab8, 0xff40596a, 0xff7a7a7a, 0xffc2c2c2,
	0xff65a0c9, 0xff6b6b84, 0xff2d2ddd, 0xff000066, 0xff0061ff, 0xff848484, 0xfff1f1df, 0xffffad7d, 0xfffbfbef,
	0xff1d830f, 0xffb0a49e, 0xff65c094, 0xff3b5985, 0xff42748d, 0xff1b8ce3, 0xff34366f, 0xff334054, 0xff45768f,

	0xffbf0a57, 0xff2198f1, 0xffffffec, 0xffb2b2b2, 0xffb2b2b2, 0xffffffff, 0xff2d5d7e, 0xff7c7c7c, 0xff7a7a7a,
	0xff7cafcf, 0xff78aaca, 0xff6a6c6d, 0xfff4efd3, 0xff28bdc4, 0xff69dd92, 0xff53ae73, 0xff0c5120, 0xff5287a5,
	0xff2a4094, 0xff7a7a7a, 0xff75718a, 0xff767676, 0xff1a162c, 0xff1a162c, 0xff1a162c, 0xff2d28a6, 0xffb1c454,
	0xff51677c, 0xff494949, 0xff343434, 0xffd18934, 0xffa5dfdd, 0xff0f090c, 0xff316397, 0xff42a0e3, 0xff4d84a1,
	0xff49859e, 0xff1f71dd, 0xffa8e2e7, 0xff74806d, 0xff3c3a2a, 0xff7c7c7c, 0xff5a5a5a, 0xff75d951, 0xff345e81,
	0xff84c0ce, 0xff455f88, 0xff868b8e, 0xffd7dd74, 0xff595959, 0xff334176, 0xff008c0a, 0xff17a404, 0xff5992b3,
	0xffb0b0b0, 0xff434347, 0xff1d6b9e, 0xff70fdfe, 0xffe5e5e5, 0xff4c4a4b, 0xffbdc6bf, 0xffddedfb, 0xff091bab,
	0xff4f547d, 0xff717171, 0xffdfe6ea, 0xffe3e8eb, 0xff41819b, 0xff747474, 0xffa1b2d1, 0xfff6f6f6, 0xff878787,
	0xff395ab0, 0xff325cac, 0xff152c47, 0xff65c878, 0xff3534df, 0xffc7c7c7, 0xffa5af72, 0xffbec7ac, 0xff9fd3dc,
	0xffcacaca, 0xff425c96, 0xff121212, 0xfff4bfa2, 0xff1474cf, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff1d56ac,

	0xff1d57ae, 0xff1d57ae, 0xff1d57ae, 0xff243c50, 0xff8dcddd, 0xff4d7aaf, 0xff0e2034, 0xff366bcf, 0xff355d7e,
	0xff7bb8c7, 0xff5f86bb, 0xff1e2e3f, 0xff3a6bc5, 0xff30536e, 0xffe0f3f7, 0xff5077a9, 0xff2955aa, 0xff21374e,
	0xffcdc5dc, 0xff603b60, 0xff856785, 0xffa679a6, 0xffaa7eaa, 0xffa879a8, 0xffa879a8, 0xffa879a8, 0xffaae6e1,
	0xffaae6e1, 0xff457d98, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff242132
};
}

size_t MCRFormat::loadPalette(const core::String &filename, io::SeekableReadStream& file, core::Array<uint32_t, 256> &palette) {
	for (int i = 0; i < lengthof(_priv::mcpalette); ++i) {
		palette[i] = _priv::mcpalette[i];
	}
	return lengthof(_priv::mcpalette);
}

bool MCRFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	enkiRegionFile regionFile;
	enkiRegionFileInit(&regionFile);
	regionFile.regionDataSize = stream.size();
	uint8_t *buffer = (uint8_t *)core_malloc(stream.size());
	stream.read(buffer, stream.size());
	regionFile.pRegionData = buffer;
	core::DynamicArray<voxel::RawVolume *> rawVolumes;

	_paletteSize = lengthof(_priv::mcpalette);
	for (size_t i = 0; i < _paletteSize; ++i) {
		_palette[i] = findClosestIndex(core::Color::fromRGBA(_priv::mcpalette[i]));
	}

	for (int i = 0; i < ENKI_MI_REGION_CHUNKS_NUMBER; i++) {
		enkiNBTDataStream stream;
		enkiInitNBTDataStreamForChunk(regionFile, i, &stream);
		if (stream.dataLength) {
			enkiChunkBlockData aChunk = enkiNBTReadChunk(&stream);
			// TODO: use aChunk.palette
			enkiMICoordinate chunkOriginPos = enkiGetChunkOrigin(&aChunk); // y always 0
			Log::debug("chunk at %d:%d:%d: Number of sections: %d", chunkOriginPos.x, chunkOriginPos.y,
					   chunkOriginPos.z, aChunk.countOfSections);

			// iterate through chunk and count non 0 voxels as a demo
			int64_t numVoxels = 0;
			for (int section = 0; section < ENKI_MI_NUM_SECTIONS_PER_CHUNK; ++section) {
				if (aChunk.sections[section]) {
					enkiMICoordinate sectionOrigin = enkiGetChunkSectionOrigin(&aChunk, section);
					Log::debug(" non empty section at %d:%d:%d", sectionOrigin.x, sectionOrigin.y,
							   sectionOrigin.z);
					enkiMICoordinate sPos;
					const glm::ivec3 mins(sectionOrigin.x, sectionOrigin.y, sectionOrigin.z);
					const glm::ivec3 size(ENKI_MI_SIZE_SECTIONS - 1, ENKI_MI_SIZE_SECTIONS - 1,
											   ENKI_MI_SIZE_SECTIONS - 1);
					const voxel::Region region(mins, mins + size);
					voxel::RawVolume *v = new voxel::RawVolume(region);
					for (sPos.y = 0; sPos.y < ENKI_MI_SIZE_SECTIONS; ++sPos.y) {
						for (sPos.z = 0; sPos.z < ENKI_MI_SIZE_SECTIONS; ++sPos.z) {
							for (sPos.x = 0; sPos.x < ENKI_MI_SIZE_SECTIONS; ++sPos.x) {
								const uint8_t color = enkiGetChunkSectionVoxel(&aChunk, section, sPos);
								if (color) {
									const uint8_t index = convertPaletteIndex(color);
									const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
									v->setVoxel(mins.x + sPos.x, mins.y + sPos.y, mins.z + sPos.z, voxel);
								}
							}
						}
					}
					rawVolumes.push_back(v);
				}
			}
			Log::debug(" chunk has %g non zero voxels\n", (float)numVoxels);

			enkiNBTRewind(&stream);
		}
		enkiNBTFreeAllocations(&stream);
	}

	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setVolume(::voxel::merge(rawVolumes), true);
	sceneGraph.emplace(core::move(node));
	for (voxel::RawVolume *v : rawVolumes) {
		delete v;
	}

	core_free(buffer);
	return true;
}

} // namespace voxel
