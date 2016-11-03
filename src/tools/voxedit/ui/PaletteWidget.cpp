#include "PaletteWidget.h"

PaletteWidget::PaletteWidget() :
		ui::Widget() {
	SetIsFocusable(true);
}

PaletteWidget::~PaletteWidget() {
}

void PaletteWidget::OnPaint(const PaintProps &paint_props) {
	Super::OnPaint(paint_props);
	tb::TBRect local_rect = GetRect();
	local_rect.x = 0;
	local_rect.y = 0;
	Log::info("render color %i:%i:%i (x:%i, y: %i, w: %i, h: %i)", _color.r, _color.g, _color.b, local_rect.x, local_rect.y, local_rect.w, local_rect.h);
	tb::g_renderer->DrawRectFill(local_rect, _color);
}

void PaletteWidget::OnInflate(const tb::INFLATE_INFO &info) {
	if (const char *colr = info.node->GetValueString("color", nullptr)) {
		_color.SetFromString(colr, strlen(colr));
	}
	Super::OnInflate(info);
}

namespace tb {
TB_WIDGET_FACTORY(PaletteWidget, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
