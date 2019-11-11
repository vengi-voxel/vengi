/**
 * @file
 */
#pragma once

#include "Widget.h"
#include "core/Common.h"

class ColorWidget: public ui::turbobadger::Widget {
public:
	UIWIDGET_SUBCLASS(ColorWidget, ui::turbobadger::Widget);

	ColorWidget();

	void setColor(const char *);
	void setColor(int r, int g, int b, int a);

	const tb::TBColor& getColor() const {
		return _color;
	}

	void setValue(int value) override;
	int getValue() const override {
		return (int) _value;
	}

	void onInflate(const tb::INFLATE_INFO &info) override;
	void onPaint(const PaintProps &paint_props) override;

private:
	tb::TBColor _color;
	uint32_t _value;
};
UIWIDGET_FACTORY(ColorWidget, tb::TBValue::TYPE_INT, tb::WIDGET_Z_TOP)

class NodeConnectorWidget: public ui::turbobadger::Widget {
public:
	UIWIDGET_SUBCLASS(NodeConnectorWidget, ui::turbobadger::Widget);

	NodeConnectorWidget();

	void onInflate(const tb::INFLATE_INFO &info) override;
	void onPaint(const PaintProps &paint_props) override;
private:
	tb::TBColor _color;
};
UIWIDGET_FACTORY(NodeConnectorWidget, tb::TBValue::TYPE_NULL, tb::WIDGET_Z_TOP)

class ImageWidget: public tb::TBImageWidget {
public:
	UIWIDGET_SUBCLASS(ImageWidget, tb::TBImageWidget);

	tb::PreferredSize onCalculatePreferredContentSize(const tb::SizeConstraints &constraints) override;
};
UIWIDGET_FACTORY(ImageWidget, tb::TBValue::TYPE_NULL, tb::WIDGET_Z_TOP)
