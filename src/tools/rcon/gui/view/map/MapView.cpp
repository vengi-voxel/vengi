/**
 * @file
 */
#include "MapView.h"
#include "AIDebugger.h"
#include "MapItem.h"
#include "Settings.h"

namespace ai {
namespace debug {

MapView::MapView(AIDebugger& debugger) :
		_debugger(debugger) {
	_scene.setItemIndexMethod(QGraphicsScene::NoIndex);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setCacheMode(QGraphicsView::CacheBackground);
	setRenderHint(QPainter::Antialiasing, false);
	setDragMode(ScrollHandDrag);
	setInteractive(true);
	setScene(&_scene);
}

MapView::~MapView() {
	_scene.clear();
}

void MapView::scalingTime(qreal /*unused*/) {
	const qreal factor = 1.0 + qreal(_numScheduledScalings) / 300.0;
	scale(factor, factor);
}

void MapView::wheelEvent(QWheelEvent * wheelEventPtr) {
	const int numDegrees = wheelEventPtr->delta() / 8;
	const int numSteps = numDegrees / 15;
	_numScheduledScalings += numSteps;

	QTimeLine *anim = new QTimeLine(350, this);
	anim->setUpdateInterval(20);
	anim->setProperty("numsteps", QVariant::fromValue(numSteps));

	connect(anim, SIGNAL(valueChanged(qreal)), SLOT(scalingTime(qreal)));
	connect(anim, SIGNAL(finished()), SLOT(animFinished()));
	anim->start();
}

void MapView::animFinished() {
	const int reduce = sender()->property("numsteps").toInt();
	_numScheduledScalings -= reduce;
	sender()->~QObject();
}

MapItem* MapView::createMapItem(const AIStateWorld& state) {
	return new MapItem(nullptr, state, _debugger);
}

MapItem* MapView::createOrUpdateMapItem(const AIStateWorld& state) {
	auto i = _items.find(state.getId());
	MapItem* item;
	if (i == _items.end()) {
		item = createMapItem(state);
	} else {
		item = i.value();
	}
	item->updateState(state);
	if (_debugger.isSelected(state)) {
		item->setZValue(std::numeric_limits<qreal>::max());
	} else {
		item->setZValue((qreal)state.getPosition().y);
	}
	if (i != _items.end()) {
		return item;
	}
	_scene.addItem(item);
	_items[state.getId()] = item;
	return item;
}

void MapView::drawBackground(QPainter* painter, const QRectF& rectf) {
	QGraphicsView::drawBackground(painter, rectf);
	const QColor& color = Settings::getBackgroundColor();
	painter->fillRect(rectf, QBrush(color));

	if (!Settings::getGrid()) {
		return;
	}

	const QColor& gridColor = Settings::getGridColor();
	QPen linePen(gridColor, 1, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
	linePen.setCosmetic(true);
	painter->setPen(linePen);

	const int gridInterval = Settings::getGridInterval();
	const qreal left = static_cast<int>(rectf.left())
			- (static_cast<int>(rectf.left()) % gridInterval);
	const qreal top = static_cast<int>(rectf.top())
			- (static_cast<int>(rectf.top()) % gridInterval);

	QVarLengthArray<QLineF, 100> linesX;
	for (qreal _x = left; _x < rectf.right(); _x += gridInterval) {
		linesX.append(QLineF(_x, rectf.top(), _x, rectf.bottom()));
	}

	QVarLengthArray<QLineF, 100> linesY;
	for (qreal _y = top; _y < rectf.bottom(); _y += gridInterval) {
		linesY.append(QLineF(rectf.left(), _y, rectf.right(), _y));
	}

	painter->drawLines(linesX.data(), linesX.size());
	painter->drawLines(linesY.data(), linesY.size());
}

void MapView::updateMapView() {
	QHash<ai::CharacterId, MapItem*> copy(_items);
	const AIDebugger::Entities& e = _debugger.getEntities();
	for (AIDebugger::EntitiesIter i = e.begin(); i != e.end(); ++i) {
		copy.remove(i->getId());
		createOrUpdateMapItem(*i);
	}

	// remove the remaining entities - they are no longer part of the snapshot
	for (auto i = copy.begin(); i != copy.end(); ++i) {
		_scene.removeItem(i.value());
		_items.remove(i.key());
	}
}

bool MapView::center(CharacterId id) {
	auto i = _items.find(id);
	if (i == _items.end()) {
		return false;
	}
	centerOn(i.value());
	return true;
}

bool MapView::makeVisible(CharacterId id) {
	auto i = _items.find(id);
	if (i == _items.end()) {
		return false;
	}
	ensureVisible(i.value());
	return true;
}

}
}
