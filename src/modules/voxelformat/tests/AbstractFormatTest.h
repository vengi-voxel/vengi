/**
 * @file
 */

#pragma once

#include "TestHelper.h"
#include "core/String.h"
#include "core/concurrent/Concurrency.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/tests/AbstractVoxelTest.h"
#include "voxelformat/Format.h"

namespace voxelformat {

class AbstractFormatTest : public voxel::AbstractVoxelTest {
private:
	void checkColor(color::RGBA, const palette::Palette &palette, uint8_t index, float maxDelta);
	static image::ImagePtr helper_testThumbnailCreator(const scenegraph::SceneGraph &sceneGraph, const ThumbnailContext &ctx);
	io::FilePtr open(const core::String &filename, io::FileMode mode = io::FileMode::Read);

	/**
	 * Helper method to load a scenegraph
	 */
	bool helper_loadIntoSceneGraph(const core::String &filename, const io::ArchivePtr &archive, Format &format, scenegraph::SceneGraph &sceneGraph);
	void testSaveLoadVolumes(const core::String &filename, const voxel::RawVolume &v, Format *format,
							voxel::ValidateFlags flags = voxel::ValidateFlags::All,
							float maxDelta = 0.001f);
	void testRGBSmall(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph);

protected:
	static const voxel::Voxel Empty;

	SaveContext testSaveCtx;
	LoadContext testLoadCtx;
	AbstractFormatTest() : voxel::AbstractVoxelTest(core::cpus()) {
		testSaveCtx.thumbnailCreator = helper_testThumbnailCreator;
	}

	io::ArchivePtr helper_archive(const core::String &filename = "");
	io::ArchivePtr helper_filesystemarchive();

	void testFirstAndLastPaletteIndex(const core::String &filename, Format *format, voxel::ValidateFlags flags);
	void testFirstAndLastPaletteIndexConversion(Format &srcFormat, const core::String &srcFilename, Format &destFormat,
												const core::String &destFilename,
												voxel::ValidateFlags flags = voxel::ValidateFlags::All);

	void testLoad(scenegraph::SceneGraph &sceneGraph, const core::String &filename, size_t expectedVolumes = 1);
	void testLoad(const core::String &filename, size_t expectedVolumes = 1) {
		scenegraph::SceneGraph sceneGraph;
		testLoad(sceneGraph, filename, expectedVolumes);
	}
	void testRGB(const core::String &filename, float maxDelta = 0.001f);
	void testRGBSmall(const core::String &filename);
	// save as the same format
	void testRGBSmallSaveLoad(const core::String &filename);
	void testLoadScreenshot(const core::String &filename, int width, int height, const color::RGBA expectedColor,
							int expectedX, int expectedY);

	// load test_material.vox and check the material for the given target format (identified by the filename)
	void testMaterial(scenegraph::SceneGraph &sceneGraph, const core::String &filename);

	void testTransform(const core::String &filename);
	void testTransform(scenegraph::SceneGraph &sceneGraph, const core::String &filename);

	// save as any other format
	void testRGBSmallSaveLoad(const core::String &filename, const core::String &saveFilename);

	void testSaveMultipleModels(const core::String &filename, Format *format,
								voxel::ValidateFlags flags = voxel::ValidateFlags::All);
	void testSaveSingleVoxel(const core::String &filename, Format *format,
							 voxel::ValidateFlags flags = voxel::ValidateFlags::All);
	void testSaveSmallVolume(const core::String &filename, Format *format,
							 voxel::ValidateFlags flags = voxel::ValidateFlags::All);

	void testSave(const core::String &filename, Format *format, const palette::Palette &palette,
				  voxel::ValidateFlags flags = voxel::ValidateFlags::All);

	void testSaveMesh(const core::String &inputFile, const core::String &filename, Format *format,
					  voxel::ValidateFlags flags = voxel::ValidateFlags::All);

	void testSaveLoadVoxel(const core::String &filename, Format *format, int mins = 0, int maxs = 1,
						   voxel::ValidateFlags flags = voxel::ValidateFlags::All, float maxDelta = 0.001f);
	void testSaveLoadCube(const core::String &filename, Format *format,
						  voxel::ValidateFlags flags = voxel::ValidateFlags::All, float maxDelta = 0.001f);
	void testConvert(const core::String &srcFilename, Format &srcFormat, const core::String &destFilename,
							 Format &destFormat, voxel::ValidateFlags flags = voxel::ValidateFlags::All,
							 float maxDelta = 0.001f);
	void testLoadSaveAndLoadSceneGraph(const core::String &srcFilename, Format &srcFormat,
									   const core::String &destFilename, Format &destFormat,
									   voxel::ValidateFlags flags = voxel::ValidateFlags::All, float maxDelta = 0.001f);
	void testConvertSceneGraph(const core::String &srcFilename1, Format &srcFormat1, const core::String &srcFilename2,
							Format &srcFormat2, voxel::ValidateFlags flags = voxel::ValidateFlags::All,
							float maxDelta = 0.001f);

	/**
	 * @brief Not a test, but a helper method to load a palette from a given format
	 */
	int helper_loadPalette(const core::String &filename, const io::ArchivePtr &archive, Format &format, palette::Palette &palette);

	/**
	 * @brief Not a test, but a helper method to store a scenegraph for manual inspection
	 */
	bool helper_saveSceneGraph(scenegraph::SceneGraph &sceneGraph, const core::String &filename);

	bool onInitApp() override;
};

} // namespace voxelformat
