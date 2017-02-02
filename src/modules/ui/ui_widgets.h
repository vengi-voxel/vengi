/**
 * @file
 */
#pragma once

#include "Widget.h"
#include "core/Common.h"

class ColorWidget: public ui::Widget {
private:
	using Super = ui::Widget;
public:
	UIWIDGET_SUBCLASS(ColorWidget, Super);

	ColorWidget();

	void SetColor(const char *);
	void SetColor(int r, int g, int b, int a);

	const tb::TBColor& GetColor() const {
		return _color;
	}

	void SetValue(int value) override;
	int GetValue() override {
		return (int) _value;
	}

	void OnInflate(const tb::INFLATE_INFO &info) override;
	void OnPaint(const PaintProps &paint_props) override;

private:
	tb::TBColor _color;
	uint32_t _value;
};

class ImageWidget: public tb::TBImageWidget {
private:
	using Super = tb::TBImageWidget;
public:
	UIWIDGET_SUBCLASS(ImageWidget, Super);

	tb::PreferredSize OnCalculatePreferredContentSize(const tb::SizeConstraints &constraints) override;
};
