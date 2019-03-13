/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Widget.h"

class LayerItem: public tb::TBGenericStringItem {
public:
	LayerItem(const char *name, bool visible = true) :
		tb::TBGenericStringItem(name, TBIDC(name)), _visible(visible) {
	}
	inline void setVisible(bool visible) {
		_visible = visible;
	}
	inline bool visible() const {
		return _visible;
	}
private:
	bool _visible;
};

class LayerItemSource: public tb::TBSelectItemSourceList<LayerItem> {
public:
	tb::TBWidget *createItemWidget(int index, tb::TBSelectItemViewer *viewer) override;
};

class LayerWidget: public tb::TBWidget {
private:
	using Super = tb::TBWidget;
public:
	TBOBJECT_SUBCLASS(LayerWidget, tb::TBWidget);

	LayerWidget();
	~LayerWidget();
	bool onEvent(const tb::TBWidgetEvent &ev) override;
	void onProcess() override;
private:
	LayerItemSource _source;
};

UIWIDGET_FACTORY(LayerWidget, tb::TBValue::TYPE_INT, tb::WIDGET_Z_TOP)
