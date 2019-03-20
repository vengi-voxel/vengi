/**
 * @file
 */

#include "LayerWidget.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "../../SceneManager.h"

class LayerItemWidget: public tb::TBLayout {
public:
	LayerItemWidget(LayerItem *item, LayerItemSource *source,
			tb::TBSelectItemViewer *sourceViewer, int index) :
			_source(source), _sourceViewer(sourceViewer), _index(index) {
		setSkinBg(TBIDC("TBSelectItem"));
		setLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
		setLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
		setPaintOverflowFadeout(false);

		core_assert_always(tb::g_widgets_reader->loadFile(getContentRoot(), "ui/widget/voxedit-layer-item.tb.txt"));
		if (tb::TBCheckBox *checkbox = getWidgetByIDAndType<tb::TBCheckBox>(TBIDC("visible"))) {
			checkbox->setValue(item->visible() ? 1 : 0);
		}
		if (tb::TBTextField *name = getWidgetByIDAndType<tb::TBTextField>(TBIDC("name"))) {
			name->setText(item->str);
		}
	}

	// TODO: allow to change the name
	bool onEvent(const tb::TBWidgetEvent &ev) override {
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("visible")) {
			LayerItem *item = _source->getItem(_index);
			item->setVisible(ev.target->getValue() ? true : false);
			_source->invokeItemChanged(_index, _sourceViewer);
			const int layerId = voxedit::sceneMgr().validLayerId(_index);
			voxedit::sceneMgr().hideLayer(layerId, !item->visible());
			return true;
		}
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("delete")) {
			const int layerId = voxedit::sceneMgr().validLayerId(_index);
			voxedit::sceneMgr().deleteLayer(layerId);
			return true;
		}
		return tb::TBLayout::onEvent(ev);
	}

	inline int index() const {
		return _index;
	}
private:
	LayerItemSource *_source;
	tb::TBSelectItemViewer *_sourceViewer;
	int _index;
};

tb::TBWidget *LayerItemSource::createItemWidget(int index, tb::TBSelectItemViewer *viewer) {
	return new LayerItemWidget(getItem(index), this, viewer, index);
}

LayerItem* LayerItemSource::getItemForLayerId(int layerId) const {
	const int n = getNumItems();
	for (int i = 0; i < n; ++i) {
		LayerItem* item = getItem(i);
		if (item->layerId() == layerId) {
			return item;
		}
	}
	return nullptr;
}

int LayerItemSource::getItemIdForLayerId(int layerId) const {
	const int n = getNumItems();
	for (int i = 0; i < n; ++i) {
		LayerItem* item = getItem(i);
		if (item->layerId() == layerId) {
			return i;
		}
	}
	return -1;
}

LayerWidget::LayerWidget() {
	core_assert_always(tb::g_widgets_reader->loadFile(getContentRoot(), "ui/widget/voxedit-layer.tb.txt"));
	_list = getWidgetByIDAndType<tb::TBSelectList>("list");
	if (_list != nullptr) {
		_list->setSource(&_source);
		_list->getScrollContainer()->setScrollMode(tb::SCROLL_MODE_Y_AUTO);
	}
	sceneMgr().registerListener(this);
}

LayerWidget::~LayerWidget() {
	if (_list != nullptr) {
		_list->setSource(nullptr);
	}
	sceneMgr().unregisterListener(this);
}

void LayerWidget::onLayerHide(int layerId) {
	const int index = _source.getItemIdForLayerId(layerId);
	if (index == -1) {
		return;
	}
	LayerItem* item = _source.getItem(index);
	if (item != nullptr) {
		item->setVisible(false);
		_source.invokeItemChanged(index, nullptr);
	}
}

void LayerWidget::onLayerShow(int layerId) {
	const int index = _source.getItemIdForLayerId(layerId);
	if (index == -1) {
		return;
	}
	LayerItem* item = _source.getItem(index);
	if (item != nullptr) {
		item->setVisible(true);
		_source.invokeItemChanged(index, nullptr);
	}
}

void LayerWidget::onActiveLayerChanged(int old, int active) {
	const int index = _source.getItemIdForLayerId(active);
	if (_list != nullptr) {
		_list->setValue(index);
	}
}

void LayerWidget::onLayerAdded(int layerId, const Layer& layer) {
	const voxedit::Layers& layers = voxedit::sceneMgr().layers();
	const std::string& finalLayerName = layers[layerId].name;
	const bool finalVisibleState = layers[layerId].visible;
	_source.addItem(new LayerItem(layerId, finalLayerName.c_str(), finalVisibleState));
}

void LayerWidget::onLayerDeleted(int layerId) {
	const int index = _source.getItemIdForLayerId(layerId);
	if (index < 0) {
		return;
	}
	_source.deleteItem(index);
}

bool LayerWidget::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("add")) {
		const tb::TBStr& name = getTextByID(TBIDC("add_layer"));
		const char *cname = name.c_str();
		const int layerId = voxedit::sceneMgr().addLayer(cname, true);
		voxedit::sceneMgr().setActiveLayer(layerId);
		return true;
	}
	if (ev.type == tb::EVENT_TYPE_CHANGED && ev.target->getID() == TBIDC("list")) {
		if (_list != nullptr) {
			voxedit::sceneMgr().setActiveLayer(_list->getValue());
		}
		return true;
	}
	return Super::onEvent(ev);
}

static LayerWidgetFactory layerWidget_wf;
