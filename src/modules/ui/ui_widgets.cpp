/**
 * @file
 */

#include "ui_widgets.h"
#include "core/Log.h"

ColorWidget::ColorWidget() :
		Super(), _color(), _value(0) {
}

void ColorWidget::SetValue(int value) {
	Log::info("SetValue to %i", value);
	if ((uint32_t)value == _value) {
		return;
	}
	_value = value;
	const int red   = (_value >> 24) & 0xFF;
	const int green = (_value >> 16) & 0xFF;
	const int blue  = (_value >>  8) & 0xFF;
	const int alpha = (_value >>  0) & 0xFF;
	_color = tb::TBColor(red, green, blue, alpha);

	InvalidateSkinStates();
	Invalidate();

	tb::TBWidgetEvent ev(tb::EVENT_TYPE_CHANGED);
	InvokeEvent(ev);
}

void ColorWidget::SetColor(const char *name) {
	if (!name) {
		return;
	}
	_color.SetFromString(name, strlen(name));
	SetValue((uint32_t)_color);
}

void ColorWidget::SetColor(int r, int g, int b, int a) {
	SetValue((uint32_t)tb::TBColor(r, g, b, a));
}

void ColorWidget::OnPaint(const PaintProps &paint_props) {
	tb::TBRect local_rect = GetRect();
	local_rect.x = 0;
	local_rect.y = 0;
	tb::g_tb_skin->PaintRectFill(local_rect, _color);
}

void ColorWidget::OnInflate(const tb::INFLATE_INFO &info) {
	if (const char *color = info.node->GetValueString("color", nullptr)) {
		SetColor(color);
	}
	Log::info("ColorWidget::OnInflate");
	Super::OnInflate(info);
}

tb::PreferredSize ImageWidget::OnCalculatePreferredContentSize(const tb::SizeConstraints &constraints) {
	const tb::PreferredSize& prefSize = Super::OnCalculatePreferredContentSize(constraints);
	if (prefSize.max_w == 0 || prefSize.max_h == 0) {
		return tb::TBWidget::OnCalculatePreferredContentSize(constraints);
	}
	return prefSize;
}
