/**
 * @file
 */

#pragma once

#include "voxedit-util/layer/LayerListener.h"
#include "ui/turbobadger/Widget.h"
#include "LayerWindow.h"

class LayerItem: public tb::TBGenericStringItem {
private:
	int _layerId;
	bool _visible;
	bool _locked;
public:
	LayerItem(int layerId, const char *name, bool visible = true, bool locked = false) :
		tb::TBGenericStringItem(name, TBIDC(name)), _layerId(layerId), _visible(visible), _locked(locked) {
	}
	inline void setVisible(bool visible) {
		_visible = visible;
	}
	inline bool visible() const {
		return _visible;
	}
	inline void setLocked(bool locked) {
		_locked = locked;
	}
	inline bool locked() const {
		return _locked;
	}
	inline int layerId() const {
		return _layerId;
	}
	inline void setLayerId(int layerId) {
		_layerId = layerId;
	}
};

class LayerItemSource: public tb::TBSelectItemSourceList<LayerItem> {
private:
	core::String _layerItemDefinition;
public:
	LayerItemSource();
	tb::TBWidget *createItemWidget(int index, tb::TBSelectItemViewer *viewer) override;
	int getItemIdForLayerId(int layerId) const;
	LayerItem* getItemForLayerId(int layerId) const;
};

class LayerWidget: public tb::TBWidget, public voxedit::LayerListener {
private:
	tb::TBSelectList *_list;
	LayerItemSource _source;
	voxedit::LayerSettings _layerSettings;
public:
	TBOBJECT_SUBCLASS(LayerWidget, tb::TBWidget);

	LayerWidget();
	~LayerWidget();
	bool onEvent(const tb::TBWidgetEvent &ev) override;

	void onLayerUnlocked(int layerId) override;
	void onLayerLocked(int layerId) override;
	void onLayerChanged(int layerId) override;
	void onLayerSwapped(int layerId1, int layerId2) override;
	void onLayerHide(int layerId) override;
	void onLayerShow(int layerId) override;
	void onActiveLayerChanged(int old, int active) override;
	void onLayerAdded(int layerId, const voxedit::Layer& layer, voxel::RawVolume* volume, const voxel::Region& region) override;
	void onLayerDeleted(int layerId, const voxedit::Layer& layer) override;
};

UIWIDGET_FACTORY(LayerWidget, tb::TBValue::TYPE_INT, tb::WIDGET_Z_TOP)
