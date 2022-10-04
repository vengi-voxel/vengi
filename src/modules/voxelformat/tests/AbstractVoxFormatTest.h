/**
 * @file
 */

#pragma once

#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/tests/AbstractVoxelTest.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/Format.h"
#include "io/Filesystem.h"
#include "core/Var.h"
#include "core/GameConfig.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/SceneGraph.h"

namespace voxelformat {

class AbstractVoxFormatTest: public voxel::AbstractVoxelTest {
protected:
	static const voxel::Voxel Empty;

	void checkColor(core::RGBA, const voxel::Palette &palette, uint8_t index, float maxDelta);

	void dump(const core::String &srcFilename, const SceneGraph &sceneGraph);
	void dump(const core::String &structName, voxel::RawVolume *v, const core::String &filename);

	void testFirstAndLastPaletteIndex(const core::String &filename, Format *format, voxel::ValidateFlags flags);
	void testFirstAndLastPaletteIndexConversion(Format &srcFormat, const core::String &destFilename, Format &destFormat,
												voxel::ValidateFlags flags = voxel::ValidateFlags::All);

	void canLoad(const core::String &filename, size_t expectedVolumes = 1);
	void testRGB(const core::String &filename, float maxDelta = 0.001f);

	void testSaveMultipleLayers(const core::String &filename, Format *format);
	void testSaveSingleVoxel(const core::String &filename, Format *format);
	void testSaveSmallVolume(const core::String &filename, Format *format);

	void testSave(const core::String &filename, Format *format);

	void testSaveLoadVoxel(const core::String &filename, Format *format, int mins = 0, int maxs = 1,
						   voxel::ValidateFlags flags = voxel::ValidateFlags::All);
	void testSaveLoadVolume(const core::String &filename, const voxel::RawVolume &v, Format *format, voxel::ValidateFlags flags = voxel::ValidateFlags::All);

	void testLoadSaveAndLoad(const core::String &srcFilename, Format &srcFormat, const core::String &destFilename,
							 Format &destFormat, voxel::ValidateFlags flags = voxel::ValidateFlags::All, float maxDelta = 0.001f);
	void testLoadSaveAndLoadSceneGraph(const core::String &srcFilename, Format &srcFormat,
									   const core::String &destFilename, Format &destFormat, voxel::ValidateFlags flags = voxel::ValidateFlags::All,
									   float maxDelta = 0.001f);

	io::FilePtr open(const core::String &filename, io::FileMode mode = io::FileMode::Read) {
		const io::FilePtr& file = io::filesystem()->open(core::String(filename), mode);
		return file;
	}

	SceneGraph::MergedVolumePalette load(const core::String& filename, io::SeekableReadStream& stream, Format& format) {
		SceneGraph sceneGraph;
		if (!format.loadGroups(filename, stream, sceneGraph)) {
			return SceneGraph::MergedVolumePalette{};
		}
		return sceneGraph.merge();
	}

	SceneGraph::MergedVolumePalette load(const core::String& filename, Format& format) {
		SceneGraph sceneGraph;
		if (!loadGroups(filename, format, sceneGraph)) {
			return SceneGraph::MergedVolumePalette{};
		}
		return sceneGraph.merge();
	}

	bool loadGroups(const core::String& filename, Format& format, voxelformat::SceneGraph &sceneGraph) {
		const io::FilePtr& file = open(filename);
		if (!file->validHandle()) {
			return false;
		}
		io::FileStream stream(file);
		return format.loadGroups(filename, stream, sceneGraph);
	}

	int loadPalette(const core::String& filename, Format& format, voxel::Palette &palette) {
		const io::FilePtr& file = open(filename);
		if (!file->validHandle()) {
			return 0;
		}
		io::FileStream stream(file);
		const int size = (int)format.loadPalette(filename, stream, palette);
		const core::String paletteFilename = core::string::extractFilename(filename) + ".png";
		palette.save(paletteFilename.c_str());
		return size;
	}

	virtual bool onInitApp() {
		if (!AbstractVoxelTest::onInitApp()) {
			return false;
		}
		FormatConfig::init();
		return true;
	}
};

}
