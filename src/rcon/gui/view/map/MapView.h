/**
 * @file
 */

#pragma once

#include <AI.h>
#include <server/AIStubTypes.h>
#include <QGraphicsScene>
#include "IGraphicsView.h"

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
class MapView: public IGraphicsView {
	Q_OBJECT
protected:
	QGraphicsScene _scene;
	AIDebugger& _debugger;
	QHash<ai::CharacterId, MapItem*> _items;
public:
	MapView(AIDebugger& debugger);
	virtual ~MapView();

	virtual void updateMapView();

	/**
	 * @brief Creates a @c MapItem and allows you to create your own instances to render extra details
	 */
	virtual MapItem* createMapItem(const AIStateWorld& state);
};

}
}
