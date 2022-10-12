/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "voxel/Palette.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/BinVoxFormat.h"
#include "voxelformat/CubFormat.h"
#include "voxelformat/GLTFFormat.h"
#include "voxelformat/GoxFormat.h"
#include "voxelformat/KV6Format.h"
#include "voxelformat/KVXFormat.h"
#include "voxelformat/OBJFormat.h"
#include "voxelformat/QBCLFormat.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/STLFormat.h"
#include "voxelformat/SproxelFormat.h"
#include "voxelformat/VXLFormat.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/VoxFormat.h"

namespace voxelformat {

class PaletteTest : public AbstractVoxFormatTest {
protected:
	bool checkNoAlpha(const voxel::Palette &palette) {
		for (int i = 0; i < palette.colorCount; ++i) {
			if (palette.colors[i].a != 255) {
				return false;
			}
		}
		return true;
	}

	// the palettes have to match, as all the colors from the rgb format are saved to the palette of the target format
	void testRGBToPaletteFormat(voxelformat::Format &rgbFormat, const core::String &rgbFile, size_t expectedColors, voxelformat::Format &paletteFormat, const core::String &palFile) {
		io::FileStream rgbStream(open(rgbFile));
		voxel::Palette rgbPalette;
		ASSERT_EQ(expectedColors, rgbFormat.loadPalette(rgbFile, rgbStream, rgbPalette));
		ASSERT_TRUE(checkNoAlpha(rgbPalette));

		rgbStream.seek(0);

		SceneGraph rgbSceneGraph;
		ASSERT_TRUE(rgbFormat.loadGroups(rgbFile, rgbStream, rgbSceneGraph)) << "Failed to load rgb model " << rgbFile;

		io::BufferedReadWriteStream palWriteStream;
		ASSERT_TRUE(paletteFormat.saveGroups(rgbSceneGraph, palFile, palWriteStream)) << "Failed to write pal model " << palFile;
		palWriteStream.seek(0);

		voxel::Palette palPalette;
		ASSERT_GT(paletteFormat.loadPalette(palFile, palWriteStream, palPalette), expectedColors);
		ASSERT_TRUE(checkNoAlpha(palPalette));

		for (size_t i = 0; i < expectedColors; ++i) {
			ASSERT_EQ(rgbPalette.colors[i], palPalette.colors[i])
				<< i << ": rgb " << core::Color::print(rgbPalette.colors[i]) << " versus pal "
				<< core::Color::print(palPalette.colors[i]) << "\n"
				<< voxel::Palette::print(rgbPalette) << "\n"
				<< voxel::Palette::print(palPalette);
		}
	}

	// the colors have to match but can differ in their count - the rgb format only saves those colors that are used by at least one voxel
	void testPaletteToRGBFormat(voxelformat::Format &paletteFormat, const core::String &palFile, voxelformat::Format &rgbFormat, const core::String &rgbFile) {
	}

	void testRGBToRGBFormat(voxelformat::Format &rgbFormat1, const core::String &rgbFile1, voxelformat::Format &rgbFormat2, const core::String &rgbFile2) {
	}

	void testPaletteToPaletteFormat(voxelformat::Format &paletteFormat1, const core::String &palFile1, voxelformat::Format &paletteFormat2, const core::String &palFile2) {
	}
};

TEST_F(PaletteTest, testQbToVox) {
	QBFormat rgb;
	VoxFormat pal;
	testRGBToPaletteFormat(rgb, "chr_knight.qb", 17, pal, "chr_knight.vox");
}

} // namespace voxelformat
