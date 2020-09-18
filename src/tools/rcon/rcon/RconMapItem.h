#pragma once

#include "MapItem.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include "shared/ProtocolEnum.h"
#include "Settings.h"
#include "core/Trace.h"

namespace rcon {

// TODO: render FIELDOFVIEW and HEALTH
class RconMapItem: public ai::debug::MapItem {
private:
	using Super = ai::debug::MapItem;

	QGraphicsEllipseItem *_visibilityCircle;
	QGraphicsEllipseItem *_attackCircle;
	QGraphicsLineItem *_healthBar;
	QGraphicsLineItem *_healthBarMax;
	QLineF _healthLine;

public:
	RconMapItem(QGraphicsItem* parent, const ai::AIStateWorld& state,
			ai::debug::AIDebugger& aiDebugger) :
			Super(parent, state, aiDebugger) {
		_visibilityCircle = new QGraphicsEllipseItem(0.0, 0.0, 0.0, 0.0, this);
		_visibilityCircle->setPen(QColor::fromRgb(255, 255, 0));
		addToGroup(_visibilityCircle);

		const qreal size = Settings::getItemSize() / 2.0;
		_healthLine = QLineF(-size, 0.0, size, 0.0);
		_healthBarMax = new QGraphicsLineItem(_healthLine, this);
		_healthBar = new QGraphicsLineItem(_healthLine, this);
		QPen pen(Qt::green);
		pen.setWidth(10);
		_healthBar->setPen(pen);
		QPen penMax(Qt::red);
		penMax.setWidth(10);
		_healthBarMax->setPen(penMax);
		addToGroup(_healthBarMax);
		addToGroup(_healthBar);

		_attackCircle = new QGraphicsEllipseItem(0.0, 0.0, 0.0, 0.0, this);
		_attackCircle->setPen(QColor::fromRgb(180, 0, 0));
		addToGroup(_attackCircle);
	}

	virtual void updateState(const ai::AIStateWorld& state) override {
		core_trace_scoped(UpdateState);
		Super::updateState(state);
		// format of those attributes is %f/%f
		updateAttrib(network::AttribType::VIEWDISTANCE, _visibilityCircle);
		updateAttrib(network::AttribType::ATTACKRANGE, _attackCircle);

		updateCurrentAndMax(network::AttribType::HEALTH, _healthBar, _healthBarMax);
	}

	void updateCurrentAndMax(network::AttribType type, QGraphicsLineItem* curItem, QGraphicsLineItem* maxItem) {
		const ai::CharacterAttributes& attributes = _state.getAttributes();
		const char *attribName = network::EnumNameAttribType(type);
		auto i = attributes.find(attribName);
		if (i != attributes.end()) {
			curItem->show();
			maxItem->show();
			const QString curAndMax(i->second.c_str());
			QStringList values = curAndMax.split("/");
			const double curVal = values[0].toDouble();
			const double maxVal = values[1].toDouble();
			const double percentage = curVal / maxVal;
			const QLineF& maxLine = maxItem->line();
			const QLineF newCurLine(maxLine.x1(), maxLine.y1(), maxLine.x2() * percentage, maxLine.y1());
			curItem->setLine(newCurLine);
		} else {
			curItem->hide();
			maxItem->hide();
		}
	}

	void updateAttrib(network::AttribType type, QGraphicsEllipseItem* item) {
		const ai::CharacterAttributes& attributes = _state.getAttributes();
		const char *attribName = network::EnumNameAttribType(type);
		auto i = attributes.find(attribName);
		if (i != attributes.end()) {
			item->show();
			const double val = ::atof(i->second.c_str());
			item->setRect(-val / 2.0, -val / 2.0, val, val);
		} else {
			item->hide();
		}
	}
};
}
