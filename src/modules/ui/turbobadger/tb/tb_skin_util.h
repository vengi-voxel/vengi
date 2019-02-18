/**
 * @file
 */

#pragma once

#include "tb_skin.h"

namespace tb {

/** Draw fade out skin elements at the edges of dst_rect if needed.
	It indicates to the user that there is hidden content.
	left, top, right, bottom specifies the (positive) distance scrolled
	from the limit. */
void drawEdgeFadeout(const TBRect &dstRect, const TBID &skinX, const TBID &skinY, int left, int top, int right,
					 int bottom);

} // namespace tb
