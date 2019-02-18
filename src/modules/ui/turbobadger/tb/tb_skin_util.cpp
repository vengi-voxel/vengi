/**
 * @file
 */

#include "tb_skin_util.h"

namespace tb {

static int getFadeoutSize(int scrolledDistance, int fadeoutLength) {
	// Make it appear gradually
	// float factor = scrolled_distance / 10.f;
	// factor = Clamp(factor, 0.5f, 1);
	// return (int)(fadeout_length * factor);
	return scrolledDistance > 0 ? fadeoutLength : 0;
}

void drawEdgeFadeout(const TBRect &dstRect, const TBID &skinX, const TBID &skinY, int left, int top, int right,
					 int bottom) {
	if (TBSkinElement *skin = g_tb_skin->getSkinElement(skinX)) {
		if (skin->bitmap != nullptr) {
			const int bw = skin->bitmap->width();
			const int bh = skin->bitmap->height();
			int dw;
			if ((dw = getFadeoutSize(left, bw)) > 0) {
				g_renderer->drawBitmap(TBRect(dstRect.x, dstRect.y, dw, dstRect.h), TBRect(0, 0, bw, bh), skin->bitmap);
			}
			if ((dw = getFadeoutSize(right, bw)) > 0) {
				g_renderer->drawBitmap(TBRect(dstRect.x + dstRect.w - dw, dstRect.y, dw, dstRect.h),
									   TBRect(bw, 0, -bw, bh), skin->bitmap);
			}
		}
	}
	if (TBSkinElement *skin = g_tb_skin->getSkinElement(skinY)) {
		if (skin->bitmap != nullptr) {
			const int bw = skin->bitmap->width();
			const int bh = skin->bitmap->height();
			int dh;
			if ((dh = getFadeoutSize(top, bh)) > 0) {
				g_renderer->drawBitmap(TBRect(dstRect.x, dstRect.y, dstRect.w, dh), TBRect(0, 0, bw, bh), skin->bitmap);
			}
			if ((dh = getFadeoutSize(bottom, bh)) > 0) {
				g_renderer->drawBitmap(TBRect(dstRect.x, dstRect.y + dstRect.h - dh, dstRect.w, dh),
									   TBRect(0, bh, bw, -bh), skin->bitmap);
			}
		}
	}
}

} // namespace tb
