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
			if (voxedit::sceneMgr().deleteLayer(layerId)) {
				_source->deleteItem(_index);
			}
			return true;
		}
		return tb::TBLayout::onEvent(ev);
	}
private:
	LayerItemSource *_source;
	tb::TBSelectItemViewer *_sourceViewer;
	int _index;
};

tb::TBWidget *LayerItemSource::createItemWidget(int index, tb::TBSelectItemViewer *viewer) {
	return new LayerItemWidget(getItem(index), this, viewer, index);
}

LayerWidget::LayerWidget() {
	core_assert_always(tb::g_widgets_reader->loadFile(getContentRoot(), "ui/widget/voxedit-layer.tb.txt"));
	if (tb::TBSelectList * select = getWidgetByIDAndType<tb::TBSelectList>("list")) {
		select->setSource(&_source);
		select->getScrollContainer()->setScrollMode(tb::SCROLL_MODE_Y_AUTO);
	}
}

LayerWidget::~LayerWidget() {
	if (tb::TBSelectList *select = getWidgetByIDAndType<tb::TBSelectList>("list")) {
		select->setSource(nullptr);
	}
}

void LayerWidget::onProcess() {
	Super::onProcess();
}

bool LayerWidget::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("add")) {
		const tb::TBStr& name = getTextByID(TBIDC("add_layer"));
		const char *cname = name.c_str();
		Log::info("Adding new layer with name '%s'", cname);
		const int layerId = voxedit::sceneMgr().addLayer(cname, true);
		if (layerId >= 0) {
			const voxedit::Layers& layers = voxedit::sceneMgr().layers();
			const std::string& finalLayerName = layers[layerId].name;
			const bool finalVisibleState = layers[layerId].visible;
			_source.addItem(new LayerItem(finalLayerName.c_str(), finalVisibleState));
			voxedit::sceneMgr().setActiveLayer(layerId);
		}
		return true;
	}
	if (ev.type == tb::EVENT_TYPE_CUSTOM && ev.isAny(TBIDC("volumeload"))) {
		const voxedit::Layers& layers = voxedit::sceneMgr().layers();
		int layerId = -1;
		_source.deleteAllItems();
		for (const auto& l : layers) {
			++layerId;
			if (!l.valid) {
				continue;
			}
			_source.addItem(new LayerItem(l.name.c_str(), l.visible));
		}
		return true;
	} else if (ev.type == tb::EVENT_TYPE_CHANGED && ev.target->getID() == TBIDC("list")) {
		if (tb::TBSelectList * select = getWidgetByIDAndType<tb::TBSelectList>("list")) {
			voxedit::sceneMgr().setActiveLayer(select->getValue());
		}
		return true;
	}
	return Super::onEvent(ev);
}

static LayerWidgetFactory layerWidget_wf;
