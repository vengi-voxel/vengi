/**
 * @file
 */

#pragma once

#include "voxedit-util/LayerListener.h"
#include "ui/turbobadger/Widget.h"
#include "LayerWindow.h"

class LayerItem: public tb::TBGenericStringItem {
public:
	LayerItem(int layerId, const char *name, bool visible = true) :
		tb::TBGenericStringItem(name, TBIDC(name)), _layerId(layerId), _visible(visible) {
	}
	inline void setVisible(bool visible) {
		_visible = visible;
	}
	inline bool visible() const {
		return _visible;
	}
	inline int layerId() const {
		return _layerId;
	}
private:
	int _layerId;
	bool _visible;
};

class LayerItemSource: public tb::TBSelectItemSourceList<LayerItem> {
public:
	tb::TBWidget *createItemWidget(int index, tb::TBSelectItemViewer *viewer) override;
	int getItemIdForLayerId(int layerId) const;
	LayerItem* getItemForLayerId(int layerId) const;
};

class LayerWidget: public tb::TBWidget, public voxedit::LayerListener {
private:
	using Super = tb::TBWidget;
public:
	TBOBJECT_SUBCLASS(LayerWidget, tb::TBWidget);

	LayerWidget();
	~LayerWidget();
	bool onEvent(const tb::TBWidgetEvent &ev) override;

	void onLayerHide(int layerId) override;
	void onLayerShow(int layerId) override;
	void onActiveLayerChanged(int old, int active) override;
	void onLayerAdded(int layerId, const voxedit::Layer& layer, voxel::RawVolume* volume, const voxel::Region& region) override;
	void onLayerDeleted(int layerId, const voxedit::Layer& layer) override;
private:
	tb::TBSelectList *_list;
	LayerItemSource _source;
	voxedit::LayerSettings _layerSettings;
};

UIWIDGET_FACTORY(LayerWidget, tb::TBValue::TYPE_INT, tb::WIDGET_Z_TOP)
