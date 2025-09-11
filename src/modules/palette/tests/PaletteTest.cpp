/**
 * @file
 */

#include "palette/Palette.h"
#include "app/tests/AbstractTest.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/ConfigVar.h"
#include "palette/FormatConfig.h"
#include "palette/PaletteLookup.h"
#include "util/VarUtil.h"

namespace palette {

class PaletteTest : public app::AbstractTest {
protected:
	bool onInitApp() override {
		if (!app::AbstractTest::onInitApp()) {
			return false;
		}
		return FormatConfig::init();
	}

	static void paletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta = 0.001f) {
		ASSERT_EQ(pal1.colorCount(), pal2.colorCount());
		// TODO: name ASSERT_EQ(pal1.name(), pal2.name());
		// TODO: materials
		for (int i = 0; i < pal1.colorCount(); ++i) {
			const core::RGBA &c1 = pal1.color(i);
			const core::RGBA &c2 = pal2.color(i);
			if (c1 != c2) {
				const float delta = core::Color::getDistance(c1, c2, core::Color::Distance::HSB);
				ASSERT_LT(delta, maxDelta) << "Palette color differs at " << i << ", color1[" << core::Color::print(c1)
										   << "], color2[" << core::Color::print(c2) << "], delta[" << delta << "]"
										   << "\nPalette 1:\n"
										   << palette::Palette::print(pal1) << "\nPalette 2:\n"
										   << palette::Palette::print(pal2);
			}
			// TODO: color names
		}
	}

	void testSaveLoad(const char *filename, float maxDelta = 0.001f) {
		Palette pal;
		pal.nippon();
		ASSERT_TRUE(pal.save(filename));
		Palette pal2;
		EXPECT_TRUE(pal2.load(filename));
		paletteComparator(pal, pal2, maxDelta);
	}
};

TEST_F(PaletteTest, testPaletteLookup) {
	palette::Palette pal;
	pal.nippon();
	palette::PaletteLookup palLookup(pal);
	const core::RGBA white(255, 255, 255, 255);
	EXPECT_EQ(0u, palLookup.findClosestIndex(white));
	const core::RGBA red(255, 0, 0, 255);
	EXPECT_EQ(37u, palLookup.findClosestIndex(red));
	const core::RGBA green(0, 255, 0, 255);
	EXPECT_EQ(149u, palLookup.findClosestIndex(green));
	const core::RGBA blue(0, 0, 255, 255);
	EXPECT_EQ(197u, palLookup.findClosestIndex(blue));
	const core::RGBA black(0, 0, 0, 255);
	EXPECT_EQ(255u, palLookup.findClosestIndex(black));
}

TEST_F(PaletteTest, testGimpRGBAPalette) {
	util::ScopedVarChange scoped(cfg::PalformatGimpRGBA, "true");
	Palette pal;
	pal.nippon();
	pal.setColor(0, core::RGBA{0, 0, 0, 127});
	const int cnt = pal.colorCount();
	ASSERT_TRUE(pal.save("test.gpl"));
	EXPECT_TRUE(pal.load("test.gpl"));
	EXPECT_EQ(pal.colorCount(), cnt);
	EXPECT_EQ(pal.color(0).a, 127);
}

TEST_F(PaletteTest, testGimpPalette) {
	testSaveLoad("test.gpl");
}

TEST_F(PaletteTest, testAVMTPalette) {
	Palette pal;
	pal.nippon();
	const int cnt = pal.colorCount();
	ASSERT_TRUE(pal.save("test.avmt"));
	Palette pal2;
	EXPECT_TRUE(pal2.load("test.avmt"));
	EXPECT_EQ(pal2.colorCount(), cnt);
}

TEST_F(PaletteTest, testAdobeColorBookPalette) {
	Palette pal;
	pal.nippon();
	pal.setName("testAdobeColorBookPalette");
	pal.setColorName(0, "Test");
	const int cnt = pal.colorCount();
	ASSERT_TRUE(pal.save("test.acb"));
	EXPECT_TRUE(pal.load("test.acb"));
	EXPECT_EQ(pal.colorCount(), cnt);
}

TEST_F(PaletteTest, testPNGPalette) {
	testSaveLoad("test.png");
}

TEST_F(PaletteTest, testPaintNetPalette) {
	Palette pal;
	EXPECT_TRUE(pal.load("paint.net.txt"));
	EXPECT_EQ(pal.colorCount(), 3);
	const uint64_t hash1 = pal.hash();

	ASSERT_TRUE(pal.save("paint.nettest.txt"));
	EXPECT_TRUE(pal.load("paint.nettest.txt"));
	EXPECT_EQ(pal.colorCount(), 3);
	const uint64_t hash2 = pal.hash();
	EXPECT_EQ(hash2, hash1);
}

TEST_F(PaletteTest, testPhotoshopPalette) {
	Palette pal;
	EXPECT_TRUE(pal.load("test.aco"));
	EXPECT_EQ(pal.colorCount(), 5);
	const uint64_t hash1 = pal.hash();

	ASSERT_TRUE(pal.save("photoshoptest.aco"));
	EXPECT_TRUE(pal.load("photoshoptest.aco"));
	EXPECT_EQ(pal.colorCount(), 5);
	const uint64_t hash2 = pal.hash();
	EXPECT_EQ(hash2, hash1);
}

TEST_F(PaletteTest, testASEPalette) {
	testSaveLoad("test.ase");
}

TEST_F(PaletteTest, testVPLPalette) {
	Palette pal;
	ASSERT_TRUE(pal.load("test.vpl"));
	EXPECT_EQ(pal.colorCount(), palette::PaletteMaxColors);
}

TEST_F(PaletteTest, testCSVPalette) {
	testSaveLoad("test.csv");
}

TEST_F(PaletteTest, testRGBPalette) {
	testSaveLoad("test.pal");
}

TEST_F(PaletteTest, testReduce) {
	Palette pal;
	pal.nippon();
	pal.reduce(16);
	EXPECT_LE(pal.colorCount(), 16);
}

TEST_F(PaletteTest, testFindReplacement) {
	Palette pal;
	pal.nippon();
	const int r1 = pal.findReplacement(0, core::Color::Distance::Approximation);
	const int r2 = pal.findReplacement(0, core::Color::Distance::HSB);
	EXPECT_NE(r1, 0);
	EXPECT_NE(r2, 0);
	const float delta = core::Color::getDistance(pal.color(r1), pal.color(r2), core::Color::Distance::HSB);
	EXPECT_LT(delta, 0.025f) << "The replacement for the first color " << core::Color::print(pal.color(0))
							 << " should be similar for both distance methods " << core::Color::print(pal.color(r1))
							 << " vs " << core::Color::print(pal.color(r2));
}

TEST_F(PaletteTest, testSaveBuiltInPalette) {
	Palette pal;
	pal.load(pal.getDefaultPaletteName());
	EXPECT_EQ("built-in:nippon", pal.name());
}

// disabled because it requires network access
TEST_F(PaletteTest, DISABLED_testLospec) {
	Palette pal;
	ASSERT_TRUE(pal.load("lospec:raspberry"));
	ASSERT_EQ(4, pal.colorCount());
}

TEST_F(PaletteTest, testCopyPalette) {
	Palette copy;
	Palette pal;
	pal.nippon();
	for (int i = 0; i < pal.colorCount(); ++i) {
		ASSERT_TRUE(copy.tryAdd(pal.color(i), false));
	}
	ASSERT_EQ(pal.colorCount(), copy.colorCount());
}

TEST_F(PaletteTest, testAddColor1) {
	int emptySlot = 0;
	Palette pal;
	EXPECT_TRUE(pal.tryAdd(core::RGBA(0), true, nullptr, false, emptySlot));
	ASSERT_EQ(1, pal.colorCount());
	EXPECT_TRUE(pal.tryAdd(core::RGBA(0, 0, 0, 255), true, nullptr, false, emptySlot));
	ASSERT_EQ(2, pal.colorCount());
}

TEST_F(PaletteTest, testAddColor2) {
	int emptySlot = 0;
	Palette pal;
	EXPECT_TRUE(pal.tryAdd(core::RGBA(0, 0, 0, 255), true, nullptr, false, emptySlot));
	ASSERT_EQ(2, pal.colorCount()) << "Empty slot was not taken into account";
	uint8_t index = -1;
	EXPECT_FALSE(pal.tryAdd(core::RGBA(0), true, &index, false, emptySlot));
	ASSERT_EQ(0, index);
}

TEST_F(PaletteTest, testGetClosestMatch) {
	Palette pal;
	pal.magicaVoxel();
	EXPECT_EQ(255, pal.colorCount());
	for (int i = 0; i < pal.colorCount(); ++i) {
		const int match = pal.getClosestMatch(pal.color(i));
		EXPECT_EQ(i, match);
	}
}

TEST_F(PaletteTest, testAddColorsNoDup) {
	Palette pal;
	const uint32_t colors[] = {
		0xff000000, 0xff7d7d7d, 0xff4cb376, 0xff436086, 0xff7a7a7a, 0xff4e7f9c, 0xff256647, 0xff535353, 0xffdcaf77,
		0xffdcaf70, 0xff135bcf, 0xff125ad4, 0xffa0d3db, 0xff7a7c7e, 0xff7c8b8f, 0xff7e8287, 0xff737373, 0xff315166,
		0xff31b245, 0xff54c3c2, 0xfff4f0da, 0xff867066, 0xff894326, 0xff838383, 0xff9fd3dc, 0xff324364, 0xff3634b4,
		0xff23c7f6, 0xff7c7c7c, 0xff77bf8e, 0xffdcdcdc, 0xff296595, 0xff194f7b, 0xff538ba5, 0xff5e96bd, 0xffdddddd};
	for (int i = 0; i < lengthof(colors); ++i) {
		EXPECT_TRUE(pal.tryAdd(colors[i], false)) << "color entry " << i << " was not added (" << colors[i] << ")";
	}
	EXPECT_EQ(lengthof(colors), pal.colorCount());
}

TEST_F(PaletteTest, testAddColorsQuantize) {
	Palette pal;
	const uint32_t colors[] = {
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
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff242132, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff,
		0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff, 0xff6699ff, 0xff3399ff,
		0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff,
		0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff, 0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff,
		0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc,
		0xff99cccc, 0xff66cccc, 0xff33cccc, 0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc,
		0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc,
		0xff9933cc, 0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc,
		0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99, 0xffcccc99,
		0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999,
		0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699, 0xff006699, 0xffff3399, 0xffcc3399,
		0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099,
		0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66, 0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66,
		0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966,
		0xff009966, 0xffff6666, 0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366,
		0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
		0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33,
		0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933, 0xff669933, 0xff339933,
		0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333,
		0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033, 0xffcc0033, 0xff990033, 0xff660033, 0xff330033,
		0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00,
		0xff99cc00, 0xff66cc00, 0xff33cc00, 0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900,
		0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300,
		0xff993300, 0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000,
		0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044, 0xff000022,
		0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400,
		0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000, 0xff880000, 0xff770000, 0xff550000,
		0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777,
		0xff555555, 0xff444444, 0xff222222, 0xff111111, 0xff00ffff, 0xffffccff, 0xffccccff};
	for (int i = 0; i < lengthof(colors); ++i) {
		pal.tryAdd(colors[i], false);
	}
	EXPECT_EQ(palette::PaletteMaxColors, pal.colorCount());
}

TEST_F(PaletteTest, testExtractPaletteName) {
	EXPECT_EQ("foobar-something", Palette::extractPaletteName("palette-foobar-something.png"));
}

TEST_F(PaletteTest, testCreateAndLoadPalette) {
	const image::ImagePtr &img = image::loadImage("test-palette-in.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	palette::Palette palette;
	EXPECT_TRUE(palette::Palette::createPalette(img, palette)) << "Failed to create palette image";
}

TEST_F(PaletteTest, testMaterialPropertyByName) {
	palette::Palette palette;
	EXPECT_FLOAT_EQ(palette.materialProperty(0, "emit"), 0.0f);
	palette.setMaterialProperty(0, "emit", 1.0f);
	EXPECT_FLOAT_EQ(palette.materialProperty(0, "emit"), 1.0f);
	palette.setMaterialValue(0, MaterialProperty::MaterialEmit, 0.5f);
	EXPECT_FLOAT_EQ(palette.materialProperty(0, "emit"), 0.5f);
}

TEST_F(PaletteTest, testPixeloramaPalette) {
	Palette pal;
	// https://raw.githubusercontent.com/Orama-Interactive/Pixelorama/refs/heads/master/pixelorama_data/Palettes/Pixelorama.json
	EXPECT_TRUE(pal.load("pixelorama.json"));
	EXPECT_EQ(56, pal.colorCount());
}

TEST_F(PaletteTest, testHasEmit) {
	Palette pal;
	pal.setColor(0, core::RGBA{255, 0, 0, 255});
	EXPECT_EQ(pal.size(), 1u);
	EXPECT_FALSE(pal.hasEmit(0));
	pal.setMaterialValue(0, MaterialEmit, 0.5f);
	EXPECT_TRUE(pal.hasEmit(0));
}

} // namespace palette
