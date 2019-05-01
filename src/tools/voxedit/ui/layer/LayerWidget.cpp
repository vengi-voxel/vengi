/**
 * @file
 */

#include "LayerWidget.h"
#include "core/App.h"
#include "core/command/CommandHandler.h"
#include "io/Filesystem.h"
#include "voxedit-util/SceneManager.h"
#include "LayerRenameWindow.h"
#include "LayerMoveWindow.h"

class LayerItemWidget: public tb::TBLayout {
public:
	LayerItemWidget(const std::string& def, LayerItem *item, LayerItemSource *source,
			tb::TBSelectItemViewer *sourceViewer) :
			_source(source), _sourceViewer(sourceViewer), _layerId(item->layerId()) {
		setSkinBg(TBIDC("TBSelectItem"));
		setLayoutDistribution(tb::LAYOUT_DISTRIBUTION_GRAVITY);
		setLayoutDistributionPosition(tb::LAYOUT_DISTRIBUTION_POSITION_LEFT_TOP);
		setPaintOverflowFadeout(false);

		core_assert_always(tb::g_widgets_reader->loadData(getContentRoot(), def.c_str()));
		if (tb::TBCheckBox *checkbox = getWidgetByIDAndType<tb::TBCheckBox>(TBIDC("visible"))) {
			checkbox->setValue(item->visible() ? 1 : 0);
		}
		if (tb::TBTextField *name = getWidgetByIDAndType<tb::TBTextField>(TBIDC("name"))) {
			name->setText(item->str);
		}
	}

	bool onEvent(const tb::TBWidgetEvent &ev) override {
		voxedit::LayerManager& layerMgr = voxedit::sceneMgr().layerMgr();
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("visible")) {
			const int itemId = _source->getItemIdForLayerId(_layerId);
			LayerItem *item = _source->getItem(itemId);
			item->setVisible(ev.target->getValue() ? true : false);
			_source->invokeItemChanged(itemId, _sourceViewer);
			layerMgr.hideLayer(_layerId, !item->visible());
			return true;
		}
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("delete")) {
			layerMgr.deleteLayer(_layerId);
			return true;
		}
		if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("layerpopupmenu")) {
			if (ev.ref_id == TBIDC("layermove")) {
				voxedit::LayerMoveWindow* win = new voxedit::LayerMoveWindow(this);
				if (!win->show()) {
					delete win;
				}
				return true;
			} else if (ev.ref_id == TBIDC("layerrename")) {
				voxedit::LayerRenameWindow* win = new voxedit::LayerRenameWindow(this);
				if (!win->show()) {
					delete win;
				}
				return true;
			}
			static const char *ACTIONS[] = {
				"layerdelete", "layerhideothers", "layerduplicate", "layershowall", "layerhideall",
				"layermoveup", "layermovedown", "layermerge",
				nullptr
			};
			for (const char** action = ACTIONS; *action != nullptr; ++action) {
				if (ev.ref_id == TBIDC(*action)) {
					core::executeCommands(*action);
					break;
				}
			}
			return true;
		}
		if (ev.type == tb::EVENT_TYPE_CONTEXT_MENU && ev.target == this) {
			layerMgr.setActiveLayer(_layerId);
			tb::TBPoint posInRoot(ev.target_x, ev.target_y);
			convertToRoot(posInRoot.x, posInRoot.y);

			if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(this, TBIDC("layerpopupmenu"))) {
				const int n = _source->getNumItems();
				tb::TBGenericStringItemSource *source = menu->getList()->getDefaultSource();
				source->addItem(new tb::TBGenericStringItem(tr("Duplicate"), TBIDC("layerduplicate")));
				source->addItem(new tb::TBGenericStringItem(tr("Move"), TBIDC("layermove")));
				source->addItem(new tb::TBGenericStringItem(tr("Rename"), TBIDC("layerrename")));
				if (n > 1) {
					source->addItem(new tb::TBGenericStringItem(tr("Delete"), TBIDC("layerdelete")));
					LayerItem* item = _source->getItemForLayerId(_layerId);
					core_assert_always(item != nullptr);
					const int n = _source->getItemIndex(item);
					Log::debug("Layer items in list: %i", n);
					core_assert(n != -1);
					if (!_source->isLast(item)) {
						source->addItem(new tb::TBGenericStringItem(tr("Merge"), TBIDC("layermerge")));
						source->addItem(new tb::TBGenericStringItem(tr("Move down"), TBIDC("layermovedown")));
					}
					if (!_source->isFirst(item)) {
						source->addItem(new tb::TBGenericStringItem(tr("Move up"), TBIDC("layermoveup")));
					}
					source->addItem(new tb::TBGenericStringItem("-"));
					source->addItem(new tb::TBGenericStringItem(tr("Hide others"), TBIDC("layerhideothers")));
				}
				source->addItem(new tb::TBGenericStringItem(tr("Show all layers"), TBIDC("layershowall")));
				source->addItem(new tb::TBGenericStringItem(tr("Hide all layers"), TBIDC("layerhideall")));
				menu->show(source, tb::TBPopupAlignment(posInRoot), -1);
			}
			return true;
		}
		return tb::TBLayout::onEvent(ev);
	}
private:
	LayerItemSource *_source;
	tb::TBSelectItemViewer *_sourceViewer;
	const int _layerId;
};

LayerItemSource::LayerItemSource() : TBSelectItemSourceList() {
	const io::FilesystemPtr& fs = core::App::getInstance()->filesystem();
	_layerItemDefinition = fs->load("ui/widget/voxedit-layer-item.tb.txt");
}

tb::TBWidget *LayerItemSource::createItemWidget(int index, tb::TBSelectItemViewer *viewer) {
	Log::debug("create LayerItemWidget at %i", index);
	return new LayerItemWidget(_layerItemDefinition, getItem(index), this, viewer);
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

static int sortByLayerId(tb::TBSelectItemSource *source, const int *a, const int *b) {
	LayerItemSource* src = (LayerItemSource*)source;
	const int layerIdA = src->getItem(*a)->layerId();
	const int layerIdB = src->getItem(*b)->layerId();
	int value;
	if (layerIdA < layerIdB) {
		value = -1;
	} else {
		core_assert(layerIdA != layerIdB);
		value = 1;
	}
	return source->getSort() == tb::TB_SORT_DESCENDING ? -value : value;
}

LayerWidget::LayerWidget() {
	_layerSettings.reset();
	core_assert_always(tb::g_widgets_reader->loadFile(getContentRoot(), "ui/widget/voxedit-layer.tb.txt"));
	_list = getWidgetByIDAndType<tb::TBSelectList>("list");
	if (_list != nullptr) {
		_source.setSort(tb::TB_SORT_ASCENDING);
		_list->setSortCallback(sortByLayerId);
		_list->setSource(&_source);
		_list->getScrollContainer()->setScrollMode(tb::SCROLL_MODE_Y_AUTO);
	}
	voxedit::sceneMgr().layerMgr().registerListener(this);
}

LayerWidget::~LayerWidget() {
	if (_list != nullptr) {
		_list->setSource(nullptr);
	}
	voxedit::sceneMgr().layerMgr().unregisterListener(this);
}

void LayerWidget::onLayerChanged(int layerId) {
	const int index = _source.getItemIdForLayerId(layerId);
	if (index == -1) {
		Log::error("Could not get item id for layer %i", layerId);
		return;
	}
	voxedit::LayerManager& layerMgr = voxedit::sceneMgr().layerMgr();
	const voxedit::Layers& layers = layerMgr.layers();
	const std::string& finalLayerName = layers[layerId].name;
	Log::debug("Rename layer %i to %s", layerId, finalLayerName.c_str());
	_source.getItem(index)->str.set(finalLayerName.c_str());
	_source.invokeItemChanged(index, _list);
	_list->invalidateList();
}

void LayerWidget::onLayerSwapped(int layerId1, int layerId2) {
	const int index1 = _source.getItemIdForLayerId(layerId1);
	if (index1 == -1) {
		Log::error("Could not get item id for layer1 %i", layerId1);
		return;
	}
	const int index2 = _source.getItemIdForLayerId(layerId2);
	if (index2 == -1) {
		Log::error("Could not get item id for layer2 %i", layerId2);
		return;
	}
	Log::debug("swap item %i and item %i", index1, index2);
	_source.getItem(index1)->setLayerId(layerId2);
	_source.getItem(index2)->setLayerId(layerId1);
	_list->invalidateList();
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
	if (_list != nullptr && index >= 0) {
		Log::debug("Item index for active layer %i is %i", active, index);
		_list->setValue(index);
	}
}

void LayerWidget::onLayerAdded(int layerId, const voxedit::Layer& layer, voxel::RawVolume*, const voxel::Region&) {
	voxedit::LayerManager& layerMgr = voxedit::sceneMgr().layerMgr();
	const voxedit::Layers& layers = layerMgr.layers();
	const std::string& finalLayerName = layers[layerId].name;
	const bool finalVisibleState = layers[layerId].visible;
	if (_source.getItemForLayerId(layerId) == nullptr) {
		_source.addItem(new LayerItem(layerId, finalLayerName.c_str(), finalVisibleState));
	}
}

void LayerWidget::onLayerDeleted(int layerId, const voxedit::Layer&) {
	const int index = _source.getItemIdForLayerId(layerId);
	if (index < 0) {
		return;
	}
	_source.deleteItem(index);
}

bool LayerWidget::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->getID() == TBIDC("layeradd")) {
		voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
		voxedit::LayerManager& layerMgr = sceneMgr.layerMgr();
		const int layerId = layerMgr.activeLayer();
		const voxel::RawVolume* v = sceneMgr.volume(layerId);
		const voxel::Region& region = v->region();
		_layerSettings.position = region.getLowerCorner();
		_layerSettings.size = region.getDimensionsInCells();
		if (_layerSettings.name.empty()) {
			_layerSettings.name = layerMgr.layer(layerId).name;
		}
		voxedit::LayerWindow* win = new voxedit::LayerWindow(this, TBIDC("scene_new_layer"), _layerSettings);
		if (!win->show()) {
			delete win;
		}
		return true;
	}

	if (ev.target->getID() == TBIDC("scene_new_layer")) {
		if (ev.ref_id == TBIDC("ok")) {
			const voxel::Region& region = _layerSettings.region();
			if (region.isValid()) {
				voxedit::LayerManager& layerMgr = voxedit::sceneMgr().layerMgr();
				voxel::RawVolume* v = new voxel::RawVolume(_layerSettings.region());
				const int layerId = layerMgr.addLayer(_layerSettings.name.c_str(), true, v, v->region().getCentre());
				layerMgr.setActiveLayer(layerId);
			} else {
				_layerSettings.reset();
			}
			return true;
		}
	}

	if (ev.type == tb::EVENT_TYPE_CHANGED && ev.target->getID() == TBIDC("list")) {
		if (_list != nullptr) {
			voxedit::LayerManager& layerMgr = voxedit::sceneMgr().layerMgr();
			const int selectedItemIndex = _list->getValue();
			const LayerItem* item = _source.getItem(selectedItemIndex);
			if (item != nullptr) {
				const int layerId = item->layerId();
				layerMgr.setActiveLayer(layerId);
			}
		}
		return true;
	}
	return Super::onEvent(ev);
}

static LayerWidgetFactory layerWidget_wf;
