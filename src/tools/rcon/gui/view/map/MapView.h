/**
 * @file
 */
#pragma once

#include <SimpleAI.h>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWheelEvent>
#include <QTimeLine>

namespace ai {
namespace debug {

class AIDebugger;
class MapItem;

/**
 * @brief The view that renders your map with all the ai controlled entities.
 *
 * @note If you want to render additional details to an entity, extend this class to override
 * MapView::createMapItem and provide your own @c MapItem there.
 */
class MapView: public QGraphicsView {
	Q_OBJECT
protected:
	QGraphicsScene _scene;
	AIDebugger& _debugger;
	QHash<ai::CharacterId, MapItem*> _items;
	int _numScheduledScalings = 0;

private slots:
	void scalingTime(qreal x);
	void animFinished();

public:
	MapView(AIDebugger& debugger);
	virtual ~MapView();

	void wheelEvent(QWheelEvent * event) override;

	virtual void updateMapView();
	bool center(CharacterId id);
	bool makeVisible(CharacterId id);

	/**
	 * @brief Creates a @c MapItem and allows you to create your own instances to render extra details
	 */
	virtual MapItem* createOrUpdateMapItem(const AIStateWorld& state);

	virtual void drawBackground(QPainter* painter, const QRectF& rect) override;
};

}
}
