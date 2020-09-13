/**
 * @file
 */

#include "ui_widgets.h"
#include "core/Log.h"

ColorWidget::ColorWidget() :
		Super(), _color(), _value(0) {
}

void ColorWidget::setValue(int value) {
	Log::info("setValue to %i", value);
	if ((uint32_t)value == _value) {
		return;
	}
	_value = value;
	const int red   = (_value >> 24) & 0xFF;
	const int green = (_value >> 16) & 0xFF;
	const int blue  = (_value >>  8) & 0xFF;
	const int alpha = (_value >>  0) & 0xFF;
	_color = tb::TBColor(red, green, blue, alpha);

	invalidateSkinStates();
	invalidate();

	tb::TBWidgetEvent ev(tb::EVENT_TYPE_CHANGED);
	invokeEvent(ev);
}

void ColorWidget::setColor(const char *name) {
	if (!name) {
		return;
	}
	_color.setFromString(name, SDL_strlen(name));
	setValue((uint32_t)_color);
}

void ColorWidget::setColor(int r, int g, int b, int a) {
	setValue((uint32_t)tb::TBColor(r, g, b, a));
}

void ColorWidget::onPaint(const PaintProps &paintProps) {
	tb::TBRect local_rect = getRect();
	local_rect.x = 0;
	local_rect.y = 0;
	tb::g_tb_skin->paintRectFill(local_rect, _color);
}

void ColorWidget::onInflate(const tb::INFLATE_INFO &info) {
	if (const char *color = info.node->getValueString("color", nullptr)) {
		setColor(color);
	}
	Super::onInflate(info);
}

NodeConnectorWidget::NodeConnectorWidget() :
		Super(), _color() {
}

void NodeConnectorWidget::onPaint(const PaintProps &paintProps) {
	tb::TBRect local_rect = getRect();
	local_rect.x = 0;
	local_rect.y = 0;
	tb::g_tb_skin->paintRectFill(local_rect, _color);
}

void NodeConnectorWidget::onInflate(const tb::INFLATE_INFO &info) {
	if (const char *color = info.node->getValueString("color", nullptr)) {
		_color.setFromString(color, SDL_strlen(color));
	}
	Super::onInflate(info);
}

tb::PreferredSize ImageWidget::onCalculatePreferredContentSize(const tb::SizeConstraints &constraints) {
	const tb::PreferredSize& prefSize = Super::onCalculatePreferredContentSize(constraints);
	if (prefSize.max_w == 0 || prefSize.max_h == 0) {
		return tb::TBImageWidget::onCalculatePreferredContentSize(constraints);
	}
	return prefSize;
}
