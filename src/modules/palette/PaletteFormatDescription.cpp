/**
 * @file
 */

#include "PaletteFormatDescription.h"
#include "io/FormatDescription.h"
#include "palette/private/ACBPalette.h"
#include "palette/private/ASEPalette.h"
#include "palette/private/AVMTPalette.h"
#include "palette/private/CSVPalette.h"
#include "palette/private/GimpPalette.h"
#include "palette/private/JASCPalette.h"
#include "palette/private/PaintNetPalette.h"
#include "palette/private/PhotoshopPalette.h"
#include "palette/private/PixeloramaPalette.h"
#include "palette/private/QBCLPalette.h"
#include "palette/private/RGBPalette.h"
#include "palette/private/VPLPalette.h"

namespace palette {

const io::FormatDescription *palettes() {
	// clang-format: off
	static thread_local io::FormatDescription desc[] = {
		ACBPalette::format(),
		ASEPalette::format(),
		AVMTPalette::format(),
		CSVPalette::format(),
		GimpPalette::format(),
		JASCPalette::format(),
		PaintNetPalette::format(),
		PhotoshopPalette::format(),
		PixeloramaPalette::format(),
		QBCLPalette::format(),
		RGBPalette::format(),
		VPLPalette::format(),
		io::format::png(),
		io::FormatDescription::END};
	// clang-format: on
	return desc;
}

} // namespace palette
