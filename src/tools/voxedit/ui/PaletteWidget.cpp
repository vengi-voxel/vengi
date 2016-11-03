#include "PaletteWidget.h"

void PaletteWidget::OnPaint(const PaintProps &paint_props) {
	tb::TBRect local_rect = GetRect();
	local_rect.x = 0;
	local_rect.y = 0;
	tb::g_renderer->DrawRectFill(local_rect, _color);
}

namespace tb {
TB_WIDGET_FACTORY(PaletteWidget, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
