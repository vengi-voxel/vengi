#pragma once

#include <QGraphicsItemGroup>
#include <QStyleOptionGraphicsItem>
#include <server/AIStubTypes.h>

namespace ai {
namespace debug {

class AIDebugger;

/**
 * @brief Represents one entity in the world
 *
 * @note If you want to show additional details for an entity, see the @c MapView class
 */
class MapItem: public QGraphicsItemGroup {
protected:
	const AIStateWorld _state;
	AIDebugger& _aiDebugger;
public:
	MapItem(QGraphicsItem* parent, const AIStateWorld& state, AIDebugger& aiDebugger);
	virtual ~MapItem();

protected:
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};

}
}
