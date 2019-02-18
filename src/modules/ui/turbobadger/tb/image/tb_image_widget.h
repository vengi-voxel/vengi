/**
 * @file
 */

#pragma once

#include "tb_image_manager.h"
#include "tb_widgets.h"

namespace tb {

/** TBImageWidget is a widget showing a image loaded by TBImageManager,
	constrained in size to its skin.
	If you need to show a image from the skin, you can use TBSkinImage. */
class TBImageWidget : public TBWidget {
public:
	// For safe typecasting
	TBOBJECT_SUBCLASS(TBImageWidget, TBWidget);

	TBImageWidget() {
	}

	void setImage(const TBImage &image) {
		m_image = image;
	}
	void setImage(const char *filename) {
		m_image = g_image_manager->getImage(filename);
	}

	virtual PreferredSize onCalculatePreferredContentSize(const SizeConstraints &constraints) override;

	virtual void onInflate(const INFLATE_INFO &info) override;
	virtual void onPaint(const PaintProps &paint_props) override;

private:
	TBImage m_image;
};

} // namespace tb
