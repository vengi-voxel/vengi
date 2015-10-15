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
	MapItem* item = new MapItem(nullptr, state, _debugger);
	item->setPos((qreal)state.getPosition().x, (qreal)state.getPosition().z);
	if (_debugger.isSelected(state)) {
		item->setZValue(std::numeric_limits<qreal>::max());
	} else {
		item->setZValue((qreal)state.getPosition().y);
	}
	return item;
}

void MapView::updateMapView() {
	_scene.clear();
	const AIDebugger::Entities& e = _debugger.getEntities();
	for (AIDebugger::EntitiesIter i = e.begin(); i != e.end(); ++i) {
		MapItem* item = createMapItem(*i);
		_scene.addItem(item);
	}
}

}
}
