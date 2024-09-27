/**
 * @file
 */

#include "PaletteFormatDescription.h"
#include "palette/private/ASEPalette.h"
#include "palette/private/CSVPalette.h"
#include "palette/private/GimpPalette.h"
#include "palette/private/JASCPalette.h"
#include "palette/private/PaintNetPalette.h"
#include "palette/private/PhotoshopPalette.h"
#include "palette/private/QBCLPalette.h"
#include "palette/private/RGBPalette.h"

namespace palette {

const io::FormatDescription *palettes() {
	// clang-format: off
	static thread_local io::FormatDescription desc[] = {
		GimpPalette::format(),
		QBCLPalette::format(),
		JASCPalette::format(),
		ASEPalette::format(),
		PhotoshopPalette::format(),
		RGBPalette::format(),
		CSVPalette::format(),
		PaintNetPalette::format(),
		io::format::png(),
		{"", {}, {}, 0u}};
	// clang-format: on
	return desc;
}

} // namespace palette
