/**
 * @file
 */
#include "NodeTreeItem.h"
#include "AIDebugger.h"
#include "TreeViewCommon.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QFont>
#include <QDateTime>

namespace ai {
namespace debug {

namespace {
const qreal padding = 1;
const qreal fontSize = 10;
const QColor backgroundColor = QColor::fromRgb(32, 32, 32, 64);
const QColor runningBackgroundColor = QColor::fromRgb(255, 0, 0, 128);
const QFont font("Times", fontSize);
const QFontMetrics fontMetrics(font);
}

NodeTreeItem::NodeTreeItem (QGraphicsItem* parentGraphicsItem, const AIStateNode& node, const AIStateNodeStatic& staticNodeData, NodeTreeItem* parent, int itemHeight, int horizontalSpacing, int verticalSpacing) :
		QGraphicsItem(parentGraphicsItem), _node(node), _parent(parent), _height(itemHeight), _horizontalSpacing(
				horizontalSpacing), _verticalSpacing(verticalSpacing) {
	_condition = QString::fromStdString(_node.getCondition());
	_name = QString::fromStdString(staticNodeData.getName());
	_type = QString::fromStdString(staticNodeData.getType());
	_width = std::max(130, std::max(fontMetrics.width(_name), fontMetrics.width(_condition)));
	_lineHeight = fontMetrics.lineSpacing();
}

NodeTreeItem::~NodeTreeItem () {
}

void NodeTreeItem::init() {
	setOffset(QPointF(100.0F, fullSize().height() / 2.0F - _height));
}

void NodeTreeItem::setOffset (const QPointF& offset) {
	_offset = offset;

	if (_parent == nullptr){
		setPos(_offset);
	} else {
		setPos(_parent->pos() + _offset);
	}

	float yOffset = 0.0F;
	foreach (NodeTreeItem* node, _children) {
		const float halfHeight = node->fullSize().height() / 2.0F;
		yOffset += halfHeight;
		float heightOffset = -_size.height() / 2.0F + yOffset;
		yOffset += halfHeight + _verticalSpacing;
		const QPointF offsetF(_width + _horizontalSpacing, heightOffset);
		node->setOffset(offsetF);
	}
}

QRectF NodeTreeItem::boundingRect () const {
	return _size;
}

QRectF NodeTreeItem::fullSize() {
	if (!_size.isEmpty()) {
		return _size;
	}

	_size = QRectF(0.0F, 0.0F, _width + _horizontalSpacing, _height + _verticalSpacing);

	if (_children.empty()) {
		return _size;
	}

	QRectF total;
	foreach (NodeTreeItem* node, _children) {
		QRectF childDimension = node->fullSize();
		childDimension.translate(node->_width + _horizontalSpacing, total.height() + childDimension.height() / 2.0F);
		total |= childDimension;
	}

	total.moveTo(total.x(), _size.center().y() - total.height() / 2.0F);

	_size |= total;

	return _size;
}

QPointF NodeTreeItem::getChildPos (NodeTreeItem* child) const {
	const QPointF& childPos = child->pos() - pos();
	const float childRelX = childPos.x();
	const float childRelY = childPos.y() + child->boundingRect().center().y();
	return QPointF(childRelX, childRelY);
}

void NodeTreeItem::paint (QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);
	//painter->setClipRect(option->exposedRect);
	const qreal lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
	const bool running = _node.isRunning();
	QBrush b = painter->brush();
	if (running) {
		painter->setBrush(runningBackgroundColor);
	} else {
		painter->setBrush(backgroundColor);
	}
	painter->drawRect(0.0F, 0.0F, _width, _height);

	if (!_children.empty()) {
		// location of the vertical line
		const float seperatorX = _width + _horizontalSpacing / 2.0F;
		// draw the (right) vertical line for connecting the parent with the separator vertical line
		painter->drawLine(_width, _height / 2.0F, seperatorX, _height / 2.0F);
		foreach (NodeTreeItem* child, _children) {
			const QPointF& childPos = getChildPos(child);
			// draw the (left) vertical line for connecting the separator with the children's left side
			painter->drawLine(seperatorX, childPos.y(), childPos.x(), childPos.y());
		}
		// draw the vertical connection line
		if (_children.size() >= 2) {
			const QPointF& firstChildPos = getChildPos(_children.first());
			const QPointF& lastChildPos = getChildPos(_children.last());
			painter->drawLine(seperatorX, firstChildPos.y(), seperatorX, lastChildPos.y());
		}
	}

	painter->setBrush(b);

	if (lod < 0.4) {
		return;
	}

	painter->setFont(font);
	painter->save();
	const int radius = 4;
	QRect rect(padding + 2 * radius, padding, _width - 2 * padding - 2 * radius, _height - 2 * padding);
	painter->drawText(rect, _name);
	rect.setY(rect.y() + _lineHeight);
	const TreeNodeStatus status = _node.getStatus();
	const QString stateString = stateNames[status];
	const int64_t lastRun = _node.getLastRun();
	painter->drawText(rect, stateString);
	QPoint center(padding + radius, padding + radius);
	rect.setY(rect.y() + _lineHeight);
	painter->drawText(rect, _condition);
	int seconds;
	if (lastRun == -1) {
		seconds = 255;
	} else {
		seconds = lastRun / 1000;
	}
	QColor activityColor(std::max(0, 255 - seconds), 0, 0, 255);
	painter->setBrush(activityColor);
	painter->drawEllipse(center, radius, radius);
	painter->restore();
}

}
}
