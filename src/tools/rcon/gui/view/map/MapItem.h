/**
 * @file
 */
#pragma once

#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
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

	QGraphicsEllipseItem *_body = nullptr;
	QGraphicsLineItem *_direction = nullptr;
	QGraphicsTextItem *_nameItem = nullptr;
public:
	MapItem(QGraphicsItem* parent, const AIStateWorld& state, AIDebugger& aiDebugger);
	virtual ~MapItem();

	void updateState(const AIStateWorld& state);

protected:
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};

}
}
