#pragma once

#include "ui/Widget.h"

class PaletteWidget: public ui::Widget {
private:
	using Super = ui::Widget;
protected:
	int _width = 0;
	int _height = 0;
	int _padding = 0;
public:
	UIWIDGET_SUBCLASS(PaletteWidget, Super);

	PaletteWidget();
	~PaletteWidget();

	void OnPaint(const PaintProps &paint_props) override;
	void OnInflate(const tb::INFLATE_INFO &info) override;
};
