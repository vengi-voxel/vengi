/**
 * @file
 */

#include "palette/ColorPalette.h"
#include "app/tests/AbstractTest.h"
#include "image/Image.h"
#include "palette/Palette.h"
#include "palette/private/PaletteFormat.h"

namespace palette {

class ColorPaletteTest : public app::AbstractTest {};

TEST_F(ColorPaletteTest, testSave) {
	palette::Palette pal;
	pal.nippon();
	ColorPalette palette = palette::toColorPalette(pal);
	EXPECT_EQ(palette.size(), pal.size());
	EXPECT_EQ(palette.name(), pal.name());
	for (size_t i = 0; i < pal.size(); ++i) {
		EXPECT_EQ(palette.color(i), pal.color(i));
	}
}

TEST_F(ColorPaletteTest, testAdd) {
	ColorPalette palette;
	palette.add(core::RGBA(255, 0, 0, 255), "Red");
	palette.add(core::RGBA(0, 255, 0, 255), "Green");
	palette.add(core::RGBA(0, 0, 255, 255), "Blue");

	EXPECT_EQ(3u, palette.size());
	EXPECT_EQ(3, palette.colorCount());
	EXPECT_EQ(core::RGBA(255, 0, 0, 255), palette.color(0));
	EXPECT_EQ("Red", palette.colorName(0));
	EXPECT_EQ(core::RGBA(0, 255, 0, 255), palette.color(1));
	EXPECT_EQ("Green", palette.colorName(1));
	EXPECT_EQ(core::RGBA(0, 0, 255, 255), palette.color(2));
	EXPECT_EQ("Blue", palette.colorName(2));
}

TEST_F(ColorPaletteTest, testSet) {
	ColorPalette palette;
	palette.setSize(2);
	palette.setColor(0, core::RGBA(255, 255, 255, 255));
	palette.setColorName(0, "White");
	palette.setColor(1, core::RGBA(0, 0, 0, 255));
	palette.setColorName(1, "Black");

	EXPECT_EQ(2u, palette.size());
	EXPECT_EQ(core::RGBA(255, 255, 255, 255), palette.color(0));
	EXPECT_EQ("White", palette.colorName(0));
	EXPECT_EQ(core::RGBA(0, 0, 0, 255), palette.color(1));
	EXPECT_EQ("Black", palette.colorName(1));

	palette.set(0, core::RGBA(127, 127, 127, 255), "Grey");
	EXPECT_EQ(core::RGBA(127, 127, 127, 255), palette.color(0));
	EXPECT_EQ("Grey", palette.colorName(0));
}

TEST_F(ColorPaletteTest, testLoad) {
	image::ImagePtr img = image::createEmptyImage("test");
	img->load(2, 2, [](int x, int y, core::RGBA &color) {
		if (x == 0 && y == 0) color = core::RGBA(255, 0, 0, 255);
		else if (x == 1 && y == 0) color = core::RGBA(0, 255, 0, 255);
		else if (x == 0 && y == 1) color = core::RGBA(0, 0, 255, 255);
		else color = core::RGBA(255, 255, 255, 255);
	});

	ColorPalette palette;
	EXPECT_TRUE(palette.load(img));
	EXPECT_EQ(4u, palette.size());
	EXPECT_EQ(core::RGBA(255, 0, 0, 255), palette.color(0));
	EXPECT_EQ(core::RGBA(0, 255, 0, 255), palette.color(1));
	EXPECT_EQ(core::RGBA(0, 0, 255, 255), palette.color(2));
	EXPECT_EQ(core::RGBA(255, 255, 255, 255), palette.color(3));
	EXPECT_EQ("test", palette.name());
}

TEST_F(ColorPaletteTest, testDirty) {
	ColorPalette palette;
	EXPECT_FALSE(palette.dirty());
	palette.add(core::RGBA(255, 0, 0, 255));
	// add does not mark dirty? Let's check implementation.
	// add calls push_back on _entries, but does not call markDirty().
	// Wait, if add doesn't mark dirty, that might be a bug or intended.
	// Let's check setSize/setColor.
	palette.markClean();
	palette.setColor(0, core::RGBA(0, 255, 0, 255));
	EXPECT_TRUE(palette.dirty());

	palette.markClean();
	palette.setName("New Name");
	EXPECT_TRUE(palette.dirty());
}

TEST_F(ColorPaletteTest, testIterators) {
	ColorPalette palette;
	palette.add(core::RGBA(255, 0, 0, 255));
	palette.add(core::RGBA(0, 255, 0, 255));

	int count = 0;
	for (const auto& entry : palette) {
		if (count == 0) {
			EXPECT_EQ(core::RGBA(255, 0, 0, 255), entry.color);
		}
		if (count == 1) {
			EXPECT_EQ(core::RGBA(0, 255, 0, 255), entry.color);
		}
		count++;
	}
	EXPECT_EQ(2, count);
}

TEST_F(ColorPaletteTest, testPrint) {
	ColorPalette palette;
	palette.add(core::RGBA(255, 0, 0, 255));
	core::String str = ColorPalette::print(palette);
	EXPECT_FALSE(str.empty());
}

} // namespace palette
