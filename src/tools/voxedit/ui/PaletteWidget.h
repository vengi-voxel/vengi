#pragma once

#include "ui/Widget.h"

class PaletteWidget: public ui::Widget {
protected:
	tb::TBColor _color;
public:
	void OnPaint(const PaintProps &paint_props);
};
