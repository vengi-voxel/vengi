// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================
//
// Copyright (c) 2016, THUNDERBEAST GAMES LLC All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "tb_widgets_reader.h"
#include "tb_widgets_common.h"
#include "tb_node_tree.h"
#include "tb_system.h"
#include "ui_widgets.h"
#include <math.h>

namespace tb {

// == TBColorWidget =======================================

TBColorWidget::TBColorWidget() :
		TBWidget(), _color(), _value(0) {
}

void TBColorWidget::SetValue(int value) {
	if (value == _value) {
		return;
	}
	_value = value;
	const int red   = (_value >> 24) & 0xFF;
	const int green = (_value >> 16) & 0xFF;
	const int blue  = (_value >>  8) & 0xFF;
	const int alpha = (_value >>  0) & 0xFF;
	_color = TBColor(red, green, blue, alpha);

	InvalidateSkinStates();
	Invalidate();

	tb::TBWidgetEvent ev(tb::EVENT_TYPE_CHANGED);
	InvokeEvent(ev);
}

void TBColorWidget::SetColor(const char *name) {
	if (!name) {
		return;
	}
	_color.SetFromString(name, strlen(name));
	SetValue((uint32)_color);
}

void TBColorWidget::SetColor(int r, int g, int b, int a) {
	SetValue((uint32)TBColor(r, g, b, a));
}

void TBColorWidget::OnPaint(const PaintProps &paint_props) {
	TBRect local_rect = GetRect();
	local_rect.x = 0;
	local_rect.y = 0;
	g_tb_skin->PaintRectFill(local_rect, _color);
}

void TBColorWidget::OnInflate(const INFLATE_INFO &info) {
	if (const char *color = info.node->GetValueString("color", nullptr)) {
		SetColor(color);
	}
	TBWidget::OnInflate(info);
}

TB_WIDGET_FACTORY(TBColorWidget, TBValue::TYPE_INT, WIDGET_Z_TOP) {}


// == TBColorWheel =======================================

TBColorWheel::TBColorWheel() :
		TBWidget(), markerx_(128), markery_(128), markercolor_(), _hue(0.0), saturation_(0.0) {
}

void TBColorWheel::OnPaint(const PaintProps &paint_props) {
	TBWidget::OnPaint(paint_props);  // draw the widget stuff

	TBRect local_rect(0, 0, 4, 4); // AND draw a marker where we clicked.
	local_rect.x = markerx_ - 2;
	local_rect.y = markery_ - 2;
	g_tb_skin->PaintRectFill(local_rect, markercolor_);
	local_rect.x -= 1;
	local_rect.y -= 1;
	local_rect.w += 2;
	local_rect.h += 2;
	g_tb_skin->PaintRectFill(local_rect, markercolor_);  // draw double box
}

bool TBColorWheel::OnEvent(const TBWidgetEvent &ev) {
	if (ev.target == this && ev.type == EVENT_TYPE_CLICK) {
		SetMarkerX(ev.target_x);
		SetMarkerY(ev.target_y);
		CalcHueSaturation(markerx_, markery_);
		TBWidgetEvent ev(EVENT_TYPE_CHANGED);
		InvokeEvent(ev);
	}
	return TBWidget::OnEvent(ev);
}

void TBColorWheel::SetHueSaturation(float hue, float saturation) {
	// suppose to set the marker position to match HS here

	_hue = hue * 360.0;
	saturation_ = saturation * 128.0;

	Invalidate();
}

void TBColorWheel::CalcHueSaturation(int rawx, int rawy) {
	TBRect rect = GetRect();
	int centerx = rect.w / 2;
	int centery = rect.h / 2;

	float X1 = rawx;
	float Y1 = rawy;
	float X2 = centerx;
	float Y2 = centery;
	float angle = 0.0;
	float xd = X2 - X1;
	float yd = Y2 - Y1;
	float dx = sqrt(xd * xd + yd * yd);

	// angle in degrees
	angle = atan2(Y2 - Y1, X2 - X1) * 180 / 3.14159265358979323846;
	if (angle < 0) {
		angle += 360.0;
	}

	// if the distance > 128, can we calculate the line point at 128 and set the marker there?

	if (dx > 128.0) {
		dx = 128.0;  // limit value
	}

	saturation_ = dx;
	_hue = angle;
}

void TBColorWheel::SetMarkerX(int value) {
	markerx_ = value;
}

void TBColorWheel::SetMarkerY(int value) {
	markery_ = value;
}

void TBColorWheel::SetMarkerColor(const char *name) {
	if (name) {
		markercolor_.SetFromString(name, strlen(name));
	}
	Invalidate();
}

void TBColorWheel::OnInflate(const INFLATE_INFO &info) {
	if (const char *colr = info.node->GetValueString("color", nullptr)) {
		SetMarkerColor(colr);
	}
	TBWidget::OnInflate(info);
}

TB_WIDGET_FACTORY(TBColorWheel, TBValue::TYPE_FLOAT, WIDGET_Z_TOP) {}

} // namespace tb
