#pragma once

#include "MapItem.h"
#include <QGraphicsEllipseItem>
#include "network/ProtocolEnum.h"

namespace rcon {

class RconMapItem: public ai::debug::MapItem {
private:
	using Super = ai::debug::MapItem;

	QGraphicsEllipseItem *_visibilityCircle = nullptr;
	QGraphicsEllipseItem *_attackCircle = nullptr;
public:
	RconMapItem(QGraphicsItem* parent, const ai::AIStateWorld& state,
			ai::debug::AIDebugger& aiDebugger) :
			Super(parent, state, aiDebugger) {
		_visibilityCircle = new QGraphicsEllipseItem(0.0, 0.0, 0.0, 0.0);
		_visibilityCircle->setPen(QColor::fromRgb(255, 255, 0));
		addToGroup(_visibilityCircle);

		_attackCircle = new QGraphicsEllipseItem(0.0, 0.0, 0.0, 0.0);
		_attackCircle->setPen(QColor::fromRgb(180, 0, 0));
		addToGroup(_attackCircle);
	}

	virtual void updateState(const ai::AIStateWorld& state) override {
		Super::updateState(state);
		// format of those attributes is %f/%f
		updateAttrib(network::AttribType::VIEWDISTANCE, _visibilityCircle);
		updateAttrib(network::AttribType::ATTACKRANGE, _attackCircle);
	}

	void updateAttrib(network::AttribType type, QGraphicsEllipseItem* item) {
		const ai::CharacterAttributes& attributes = _state.getAttributes();
		const char *attribName = network::EnumNameAttribType(type);
		auto i = attributes.find(attribName);
		if (i != attributes.end()) {
			const double val = ::atof(i->second.c_str());
			item->setRect(-val / 2.0, -val / 2.0, val, val);
		}
	}
};
}
