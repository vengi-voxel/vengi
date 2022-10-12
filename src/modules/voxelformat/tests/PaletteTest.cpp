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

	bool findColor(const voxel::Palette &palette, core::RGBA rgba) {
		for (int i = 0; i < palette.colorCount; ++i) {
			if (palette.colors[i] == rgba) {
				return true;
			}
		}
		return false;
	}

	// the palettes have to match, as all the colors from the rgb format are saved to the palette of the target format
	void testRGBToPaletteFormat(voxelformat::Format &rgbFormat, const core::String &rgbFile, size_t rgbExpectedColors, voxelformat::Format &paletteFormat, const core::String &palFile, size_t palExpectedColors) {
		io::FileStream rgbStream(open(rgbFile));
		voxel::Palette rgbPalette;
		ASSERT_EQ(rgbExpectedColors, rgbFormat.loadPalette(rgbFile, rgbStream, rgbPalette));
		ASSERT_TRUE(checkNoAlpha(rgbPalette));

		rgbStream.seek(0);

		SceneGraph rgbSceneGraph;
		ASSERT_TRUE(rgbFormat.loadGroups(rgbFile, rgbStream, rgbSceneGraph)) << "Failed to load rgb model " << rgbFile;

		io::BufferedReadWriteStream palWriteStream;
		ASSERT_TRUE(paletteFormat.saveGroups(rgbSceneGraph, palFile, palWriteStream)) << "Failed to write pal model " << palFile;
		palWriteStream.seek(0);

		voxel::Palette palPalette;
		ASSERT_EQ(paletteFormat.loadPalette(palFile, palWriteStream, palPalette), palExpectedColors);
		ASSERT_TRUE(checkNoAlpha(palPalette));

		for (size_t i = 0; i < rgbExpectedColors; ++i) {
			ASSERT_EQ(rgbPalette.colors[i], palPalette.colors[i])
				<< i << ": rgb " << core::Color::print(rgbPalette.colors[i]) << " versus pal "
				<< core::Color::print(palPalette.colors[i]) << "\n"
				<< voxel::Palette::print(rgbPalette) << "\n"
				<< voxel::Palette::print(palPalette);
		}
	}

	// the colors have to match but can differ in their count - the rgb format only saves those colors that are used by at least one voxel
	void testPaletteToRGBFormat(voxelformat::Format &palFormat, const core::String &palFile, size_t palExpectedColors, voxelformat::Format &rgbFormat, const core::String &rgbFile, size_t rgbExpectedColors) {
		io::FileStream palStream(open(palFile));
		voxel::Palette palPalette;
		ASSERT_EQ(palExpectedColors, palFormat.loadPalette(palFile, palStream, palPalette));
		ASSERT_TRUE(checkNoAlpha(palPalette));

		palStream.seek(0);

		SceneGraph palSceneGraph;
		ASSERT_TRUE(palFormat.loadGroups(palFile, palStream, palSceneGraph)) << "Failed to load pal model " << palFile;

		io::BufferedReadWriteStream rgbWriteStream;
		ASSERT_TRUE(rgbFormat.saveGroups(palSceneGraph, rgbFile, rgbWriteStream)) << "Failed to write rgb model " << rgbFile;
		rgbWriteStream.seek(0);

		voxel::Palette rgbPalette;
		ASSERT_EQ(rgbFormat.loadPalette(rgbFile, rgbWriteStream, rgbPalette), rgbExpectedColors);
		ASSERT_TRUE(checkNoAlpha(rgbPalette));

		for (size_t i = 0; i < rgbExpectedColors; ++i) {
			ASSERT_TRUE(findColor(palPalette, rgbPalette.colors[i]))
				<< i << ": Could not find color " << core::Color::print(rgbPalette.colors[i]) << " in pal palette\n"
				<< voxel::Palette::print(palPalette);
		}
	}

	void testRGBToRGBFormat(voxelformat::Format &rgbFormat1, const core::String &rgbFile1, voxelformat::Format &rgbFormat2, const core::String &rgbFile2, size_t expectedColors) {
	}

	void testPaletteToPaletteFormat(voxelformat::Format &palFormat1, const core::String &palFile1, voxelformat::Format &palFormat2, const core::String &palFile2, size_t expectedColors) {
	}
};

TEST_F(PaletteTest, testQbToVox) {
	QBFormat rgb;
	VoxFormat pal;
	testRGBToPaletteFormat(rgb, "chr_knight.qb", 17, pal, "chr_knight-qbtovox.vox", 256);
}

TEST_F(PaletteTest, testQbToQb) {
	QBFormat rgb1;
	QBFormat rgb2;
	testRGBToRGBFormat(rgb1, "chr_knight.qb", rgb2, "chr_knight-testqbtoqb.qb", 17);
}

TEST_F(PaletteTest, testVoxToVox) {
	VoxFormat pal1;
	VoxFormat pal2;
	testPaletteToPaletteFormat(pal1, "magicavoxel.vox", pal2, "magicavoxel-testvoxtovox.qb", 256);
}

TEST_F(PaletteTest, testVoxToQb) {
	QBFormat rgb;
	VoxFormat pal;
	testPaletteToRGBFormat(pal, "magicavoxel.vox", 256, rgb, "magicavoxel-testvoxtoqb.qb", 21);
}

} // namespace voxelformat
