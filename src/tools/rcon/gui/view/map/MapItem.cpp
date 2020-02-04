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
#include <math.h>
#include <QVector2D>

namespace ai {
namespace debug {

namespace {
const float OrientationScale = 2.0F;
}

MapItem::MapItem(QGraphicsItem* parent, const AIStateWorld& state, AIDebugger& aiDebugger) :
		QGraphicsItemGroup(parent), _state(state), _aiDebugger(aiDebugger) {
	setFlag(QGraphicsItem::ItemIsSelectable);

	_body = new QGraphicsEllipseItem(0.0, 0.0, 0.0, 0.0, this);
	_direction = new QGraphicsLineItem(0.0, 0.0, 0.0, 0.0, this);
	_nameItem = new QGraphicsTextItem(_body);
	_nameItem->setDefaultTextColor(Settings::getNameColor());

	addToGroup(_body);
	addToGroup(_direction);
	addToGroup(_nameItem);

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::AllButtons);
}

MapItem::~MapItem() {
}

void MapItem::updateState(const AIStateWorld& state) {
	const qreal size = Settings::getItemSize();
	setPos((qreal)state.getPosition().x, (qreal)state.getPosition().z);
	_body->setRect(-size / 2.0, -size / 2.0, size, size);

	const CharacterAttributes& attributes = _state.getAttributes();
	const QString& nameAttribute = Settings::getNameAttribute(attributes::NAME);
	CharacterAttributes::const_iterator name = attributes.find(nameAttribute.toStdString().c_str());
	if (name != attributes.end()) {
		const QString nameStr(name->second.c_str());
		setToolTip(nameStr);
		_nameItem->setPlainText(nameStr);
	} else {
		const QString& nameStr = QString::number(_state.getId());
		setToolTip(nameStr);
		_nameItem->setPlainText(nameStr);
	}

	const bool selected = _aiDebugger.isSelected(state);
	if (selected) {
		static const QPen pen(QColor::fromRgb(255, 0, 0, 255));
		_body->setPen(pen);
	} else {
		static const QPen pen(QColor::fromRgb(0, 0, 0, 255));
		_body->setPen(pen);
	}
	// TODO: get color from settings
	QColor color = QColor::fromRgb(200, 200, 0, 255);
	_body->setBrush(color);

	QVector2D end(::cosf(state.getOrientation()) * OrientationScale, ::sinf(state.getOrientation()) * OrientationScale);
	end.normalize();
	const qreal center = size / 2.0;
	_direction->setLine(0.0, 0.0, center * end.x(), center * end.y());

	const QString& groupAttribute = Settings::getGroupAttribute(attributes::GROUP);
	CharacterAttributes::const_iterator groupIter = attributes.find(groupAttribute.toStdString().c_str());
	if (groupIter != attributes.end()) {
		// TODO: get color from settings
		QColor colorGroup = QColor::fromRgb(200, 200, 0, 255);
		const int groupId = atoi(groupIter->second.c_str());
		if (groupId > 0) {
			const int b = groupId * 113 % 255;
			const int component = groupId % 3;
			switch (component) {
			case 0:
				colorGroup.setRed(b);
				break;
			case 1:
				colorGroup.setGreen(b);
				break;
			default:
				colorGroup.setBlue(b);
				break;
			}
		}
		_body->setBrush(colorGroup);
	}
}

void MapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	QGraphicsItem::mouseDoubleClickEvent(event);
	_aiDebugger.select(_state);
	update();
}

}
}
