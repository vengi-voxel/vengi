/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "palette/Palette.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/qubicle/QBCLFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class FormatPaletteTest : public AbstractFormatTest {
protected:
	bool checkNoAlpha(const palette::Palette &palette) {
		for (int i = 0; i < palette.colorCount(); ++i) {
			if (palette.color(i).a != 255) {
				return false;
			}
		}
		return true;
	}

	// the palettes have to match, as all the colors from the rgb format are saved to the palette of the target format
	void testRGBToPaletteFormat(voxelformat::Format &rgbFormat, const core::String &rgbFile, size_t rgbExpectedColors,
								voxelformat::Format &paletteFormat, const core::String &palFile,
								voxel::ValidateFlags flags = voxel::ValidateFlags::PaletteMinMatchingColors,
								float maxDelta = 0.00001f) {
		SCOPED_TRACE("rgb file: " + rgbFile);
		SCOPED_TRACE("pal file: " + palFile);

		io::ArchivePtr archive = helper_filesystemarchive();
		palette::Palette rgbPalette;
		EXPECT_EQ(rgbExpectedColors, rgbFormat.loadPalette(rgbFile, archive, rgbPalette, testLoadCtx))
			<< "Found expected amount of colors in the rgb format";
		ASSERT_TRUE(checkNoAlpha(rgbPalette)) << "Found alpha in the rgb palette";

		scenegraph::SceneGraph rgbSceneGraph;
		ASSERT_TRUE(rgbFormat.load(rgbFile, archive, rgbSceneGraph, testLoadCtx))
			<< "Failed to load rgb model " << rgbFile;

		ASSERT_TRUE(paletteFormat.save(rgbSceneGraph, palFile, archive, testSaveCtx))
			<< "Failed to write pal model " << palFile;

		palette::Palette palPalette;
		ASSERT_GT(paletteFormat.loadPalette(palFile, archive, palPalette, testLoadCtx), 0u)
			<< "Found expected amount of colors in the palette format";
		// ASSERT_TRUE(checkNoAlpha(palPalette));

		if ((flags & voxel::ValidateFlags::Palette) == voxel::ValidateFlags::Palette) {
			voxel::paletteComparator(palPalette, rgbPalette, maxDelta);
		} else if ((flags & voxel::ValidateFlags::PaletteMinMatchingColors) ==
				   voxel::ValidateFlags::PaletteMinMatchingColors) {
			voxel::partialPaletteComparator(palPalette, rgbPalette, flags, maxDelta);
		} else if ((flags & voxel::ValidateFlags::PaletteColorsScaled) == voxel::ValidateFlags::PaletteColorsScaled) {
			voxel::paletteComparatorScaled(palPalette, rgbPalette, (int)maxDelta);
		} else if ((flags & voxel::ValidateFlags::PaletteColorOrderDiffers) ==
				   voxel::ValidateFlags::PaletteColorOrderDiffers) {
			voxel::orderPaletteComparator(palPalette, rgbPalette, maxDelta);
		}

		for (size_t i = 0; i < rgbExpectedColors; ++i) {
			ASSERT_EQ(rgbPalette.color(i), palPalette.color(i))
				<< i << ": rgb " << color::Color::print(rgbPalette.color(i)) << " versus pal "
				<< color::Color::print(palPalette.color(i)) << "\n"
				<< palette::Palette::print(rgbPalette) << "\n"
				<< palette::Palette::print(palPalette);
		}
	}

	// the colors have to match but can differ in their count - the rgb format only saves those colors that are used by
	// at least one voxel
	void testPaletteToRGBFormat(voxelformat::Format &palFormat, const core::String &palFile, size_t palExpectedColors,
								voxelformat::Format &rgbFormat, const core::String &rgbFile, size_t rgbExpectedColors) {
		SCOPED_TRACE("pal file: " + palFile);
		SCOPED_TRACE("rgb file: " + rgbFile);

		io::ArchivePtr archive = helper_filesystemarchive();

		palette::Palette palPalette;
		ASSERT_EQ(palExpectedColors, palFormat.loadPalette(palFile, archive, palPalette, testLoadCtx));
		// ASSERT_TRUE(checkNoAlpha(palPalette));

		scenegraph::SceneGraph palSceneGraph;
		ASSERT_TRUE(palFormat.load(palFile, archive, palSceneGraph, testLoadCtx))
			<< "Failed to load pal model " << palFile;

		ASSERT_TRUE(rgbFormat.save(palSceneGraph, rgbFile, archive, testSaveCtx))
			<< "Failed to write rgb model " << rgbFile;

		palette::Palette rgbPalette;
		ASSERT_EQ(rgbFormat.loadPalette(rgbFile, archive, rgbPalette, testLoadCtx), rgbExpectedColors);
		ASSERT_TRUE(checkNoAlpha(rgbPalette));

		for (size_t i = 0; i < rgbExpectedColors; ++i) {
			ASSERT_TRUE(palPalette.hasColor(rgbPalette.color(i)))
				<< i << ": Could not find color " << color::Color::print(rgbPalette.color(i)) << " in pal palette\n"
				<< palette::Palette::print(palPalette);
		}
	}

	void testRGBToRGBFormat(voxelformat::Format &rgbFormat1, const core::String &rgbFile1,
							voxelformat::Format &rgbFormat2, const core::String &rgbFile2, size_t expectedColors) {
		SCOPED_TRACE("1. rgb file: " + rgbFile1);
		SCOPED_TRACE("2. rgb file: " + rgbFile2);

		io::ArchivePtr archive = helper_filesystemarchive();

		palette::Palette rgbPalette1;
		ASSERT_EQ(expectedColors, rgbFormat1.loadPalette(rgbFile1, archive, rgbPalette1, testLoadCtx));
		ASSERT_TRUE(checkNoAlpha(rgbPalette1));

		scenegraph::SceneGraph palSceneGraph;
		ASSERT_TRUE(rgbFormat1.load(rgbFile1, archive, palSceneGraph, testLoadCtx))
			<< "Failed to load rgb model " << rgbFile1;

		ASSERT_TRUE(rgbFormat2.save(palSceneGraph, rgbFile2, archive, testSaveCtx))
			<< "Failed to write rgb model " << rgbFile2;

		palette::Palette rgbPalette2;
		ASSERT_EQ(rgbFormat2.loadPalette(rgbFile2, archive, rgbPalette2, testLoadCtx), expectedColors);
		ASSERT_TRUE(checkNoAlpha(rgbPalette2));

		// the colors might have a different ordering here it depends on the order we read the volume for the rgb format
		for (size_t i = 0; i < expectedColors; ++i) {
			ASSERT_TRUE(rgbPalette1.hasColor(rgbPalette2.color(i)))
				<< i << ": Could not find color " << color::Color::print(rgbPalette2.color(i)) << " in rgb palette\n"
				<< palette::Palette::print(rgbPalette1);
		}
	}

	void testPaletteToPaletteFormat(voxelformat::Format &palFormat1, const core::String &palFile1,
									voxelformat::Format &palFormat2, const core::String &palFile2,
									size_t expectedColors) {
		SCOPED_TRACE("1. pal file: " + palFile1);
		SCOPED_TRACE("2. pal file: " + palFile2);

		io::ArchivePtr archive = helper_filesystemarchive();

		palette::Palette palPalette1;
		ASSERT_EQ(expectedColors, palFormat1.loadPalette(palFile1, archive, palPalette1, testLoadCtx));
		// ASSERT_TRUE(checkNoAlpha(palPalette1));

		scenegraph::SceneGraph palSceneGraph;
		ASSERT_TRUE(palFormat1.load(palFile1, archive, palSceneGraph, testLoadCtx))
			<< "Failed to load pal model " << palFile1;

		ASSERT_TRUE(palFormat2.save(palSceneGraph, palFile2, archive, testSaveCtx))
			<< "Failed to write pal model " << palFile2;

		palette::Palette palPalette2;
		ASSERT_EQ(palFormat2.loadPalette(palFile2, archive, palPalette2, testLoadCtx), expectedColors);
		// ASSERT_TRUE(checkNoAlpha(palPalette2));

		for (size_t i = 0; i < expectedColors; ++i) {
			ASSERT_EQ(palPalette1.color(i), palPalette2.color(i))
				<< i << ": pal " << color::Color::print(palPalette1.color(i)) << " versus pal "
				<< color::Color::print(palPalette2.color(i)) << "\n"
				<< palette::Palette::print(palPalette1) << "\n"
				<< palette::Palette::print(palPalette2);
		}
	}
};

TEST_F(FormatPaletteTest, testQbToVox) {
	QBFormat rgb;
	VoxFormat pal;
	testRGBToPaletteFormat(rgb, "chr_knight.qb", 17, pal, "chr_knight-qbtovox.vox");
}

TEST_F(FormatPaletteTest, testQbToQb) {
	QBFormat rgb1;
	QBFormat rgb2;
	testRGBToRGBFormat(rgb1, "chr_knight.qb", rgb2, "chr_knight-testqbtoqb.qb", 17);
}

TEST_F(FormatPaletteTest, testQbToQBCL) {
	QBFormat rgb1;
	QBCLFormat rgb2;
	testRGBToRGBFormat(rgb1, "chr_knight.qb", rgb2, "chr_knight-testqbtoqb.qbcl", 17);
}

TEST_F(FormatPaletteTest, testVoxToVox) {
	VoxFormat pal1;
	VoxFormat pal2;
	testPaletteToPaletteFormat(pal1, "magicavoxel.vox", pal2, "magicavoxel-testvoxtovox.qb", 255);
}

TEST_F(FormatPaletteTest, testVoxToQb) {
	QBFormat rgb;
	VoxFormat pal;
	testPaletteToRGBFormat(pal, "magicavoxel.vox", 255, rgb, "magicavoxel-testvoxtoqb.qb", 21);
}

} // namespace voxelformat
