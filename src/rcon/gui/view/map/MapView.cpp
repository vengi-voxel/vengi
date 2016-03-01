#include "MapView.h"
#include "AIDebugger.h"
#include "MapItem.h"

namespace ai {
namespace debug {

MapView::MapView(AIDebugger& debugger) :
		IGraphicsView(true, true), _debugger(debugger) {
	_scene.setItemIndexMethod(QGraphicsScene::NoIndex);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setCacheMode(QGraphicsView::CacheBackground);
	setRenderHint(QPainter::Antialiasing, false);
	setInteractive(true);
	setScene(&_scene);
}

MapView::~MapView() {
	_scene.clear();
}

MapItem* MapView::createMapItem(const AIStateWorld& state) {
	auto i = _items.find(state.getId());
	MapItem* item;
	if (i != _items.end()) {
		item = i.value();
	} else {
		item = new MapItem(nullptr, state, _debugger);
	}
	item->setPos((qreal)state.getPosition().x, (qreal)state.getPosition().z);
	if (_debugger.isSelected(state)) {
		item->setZValue(std::numeric_limits<qreal>::max());
	} else {
		item->setZValue((qreal)state.getPosition().y);
	}
	if (i == _items.end())
		return nullptr;
	_items[state.getId()] = item;
	return item;
}

void MapView::updateMapView() {
	QHash<ai::CharacterId, MapItem*> copy(_items);
	const AIDebugger::Entities& e = _debugger.getEntities();
	for (AIDebugger::EntitiesIter i = e.begin(); i != e.end(); ++i) {
		copy.remove(i->getId());
		MapItem* item = createMapItem(*i);
		if (item == nullptr)
			continue;
		_scene.addItem(item);
	}

	// remove the remaining entities - they are no longer part of the snapshot
	for (auto i = copy.begin(); i != copy.end(); ++i) {
		_scene.removeItem(i.value());
		_items.remove(i.key());
	}
}

}
}
