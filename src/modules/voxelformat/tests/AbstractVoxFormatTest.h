/**
 * @file
 */

#pragma once

#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "voxel/RawVolume.h"
#include "voxel/tests/AbstractVoxelTest.h"
#include "voxelformat/Format.h"
#include "io/Filesystem.h"
#include "core/Var.h"
#include "core/GameConfig.h"

namespace voxel {

class AbstractVoxFormatTest: public AbstractVoxelTest {
protected:
	static const voxel::Voxel Empty;

	void dump(const core::String& srcFilename, const voxel::SceneGraph &sceneGraph);
	void dump(const core::String& structName, voxel::RawVolume* v, const core::String& filename);

	void testFirstAndLastPaletteIndex(const core::String &filename, voxel::Format *format, bool includingColor,
									  bool includingRegion);
	void testFirstAndLastPaletteIndexConversion(voxel::Format &srcFormat, const core::String &destFilename,
												voxel::Format &destFormat, bool includingColor, bool includingRegion);

	void testRGB(RawVolume *volume);

	void testSaveMultipleLayers(const core::String& filename, voxel::Format* format);

	void testSave(const core::String& filename, voxel::Format* format);

	void testSaveLoadVoxel(const core::String& filename, voxel::Format* format, int mins = 0, int maxs = 1);

	void testLoadSaveAndLoad(const core::String &srcFilename, voxel::Format &srcFormat,
							 const core::String &destFilename, voxel::Format &destFormat, bool includingColor,
							 bool includingRegion);

	io::FilePtr open(const core::String& filename, io::FileMode mode = io::FileMode::Read) {
		const io::FilePtr& file = io::filesystem()->open(core::String(filename), mode);
		return file;
	}

	voxel::RawVolume* load(const core::String& filename, voxel::Format& format) {
		const io::FilePtr& file = open(filename);
		if (!file->validHandle()) {
			return nullptr;
		}
		io::FileStream stream(file);
		voxel::RawVolume* v = format.load(filename, stream);
		return v;
	}

	int loadPalette(const core::String& filename, voxel::Format& format, voxel::Palette &palette) {
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
		core::Var::get(cfg::VoxformatMergequads, "true", core::CV_NOPERSIST, "Merge similar quads to optimize the mesh");
		core::Var::get(cfg::VoxformatReusevertices, "true", core::CV_NOPERSIST, "Reuse vertices or always create new ones");
		core::Var::get(cfg::VoxformatAmbientocclusion, "false", core::CV_NOPERSIST, "Extra vertices for ambient occlusion");
		core::Var::get(cfg::VoxformatScale, "1.0", core::CV_NOPERSIST, "Scale the vertices by the given factor");
		core::Var::get(cfg::VoxformatScaleX, "1.0", core::CV_NOPERSIST, "Scale the vertices on X axis by the given factor");
		core::Var::get(cfg::VoxformatScaleY, "1.0", core::CV_NOPERSIST, "Scale the vertices on Y axis by the given factor");
		core::Var::get(cfg::VoxformatScaleZ, "1.0", core::CV_NOPERSIST, "Scale the vertices on Z axis by the given factor");
		core::Var::get(cfg::VoxformatFrame, "0", core::CV_NOPERSIST, "Which frame to import for formats that support this - starting at 0");
		core::Var::get(cfg::VoxformatQuads, "true", core::CV_NOPERSIST, "Export as quads. If this false, triangles will be used.");
		core::Var::get(cfg::VoxformatWithcolor, "true", core::CV_NOPERSIST, "Export with vertex colors");
		core::Var::get(cfg::VoxformatWithtexcoords, "true", core::CV_NOPERSIST, "Export with uv coordinates of the palette image");
		core::Var::get(cfg::VoxformatTransform, "false", core::CV_NOPERSIST, "Apply the scene graph transform to mesh exports");
		return true;
	}
};

}
