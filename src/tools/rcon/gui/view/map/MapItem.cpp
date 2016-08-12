/**
 * @file
 */
#include "MapItem.h"
#include "AIDebugger.h"
#include "Settings.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <cmath>
#include <QVector2D>

namespace ai {
namespace debug {

MapItem::MapItem(QGraphicsItem* parent, const AIStateWorld& state, AIDebugger& aiDebugger) :
		QGraphicsItemGroup(parent), _state(state), _aiDebugger(aiDebugger) {
	const bool selected = _aiDebugger.isSelected(state);
	setFlag(QGraphicsItem::ItemIsSelectable);

	const qreal size = Settings::getItemSize();
	QColor color = selected ? QColor::fromRgb(200, 200, 0, 255) : QColor::fromRgb(255, 0, 0, 128);
	QGraphicsEllipseItem *body = new QGraphicsEllipseItem(0.0, 0.0, size, size);
	addToGroup(body);

	QVector2D end(::cosf(state.getOrientation()), ::sinf(state.getOrientation()));
	end.normalize();
	const qreal center = size / 2.0;
	QGraphicsLineItem *direction = new QGraphicsLineItem(center, center, center + center * end.x(), center + center * end.y());
	addToGroup(direction);

	direction->setSelected(selected);
	body->setSelected(selected);

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::AllButtons);
	const CharacterAttributes& attributes = _state.getAttributes();
	CharacterAttributes::const_iterator name = attributes.find(attributes::NAME);
	if (name != attributes.end()) {
		setToolTip(QString::fromStdString(name->second));
	} else {
		setToolTip(QString::number(_state.getId()));
	}
	CharacterAttributes::const_iterator group = attributes.find(attributes::GROUP);
	if (group != attributes.end()) {
		const int groupId = atoi(group->second.c_str());
		if (groupId > 0) {
			const int b = groupId * 113 % 255;
			const int component = groupId % 3;
			switch (component) {
			case 0:
				color.setRed(b);
				break;
			case 1:
				color.setGreen(b);
				break;
			default:
				color.setBlue(b);
				break;
			}
		}
	}

	body->setBrush(color);
}

MapItem::~MapItem() {
}

void MapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	QGraphicsItem::mouseDoubleClickEvent(event);
	_aiDebugger.select(_state);
	update();
}

}
}
