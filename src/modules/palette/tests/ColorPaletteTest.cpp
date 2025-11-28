/**
 * @file
 */

#include "palette/ColorPalette.h"
#include "app/tests/AbstractTest.h"
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

} // namespace palette
