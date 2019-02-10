/**
 * @file
 */
#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QTimeLine>
#include "NodeTreeItem.h"

namespace ai {
namespace debug {

class AINodeStaticResolver;
class AIDebugger;

/**
 * @brief Shows the behaviour tree for the current selected entity
 */
class NodeTreeView: public QGraphicsView {
Q_OBJECT
private:
	AIDebugger& _debugger;
	QGraphicsScene _scene;
	AINodeStaticResolver& _resolver;

	NodeTreeItem* buildTreeItems(const AIStateNode& node, NodeTreeItem* parent);
	int _numScheduledScalings = 0;

private slots:
	void scalingTime(qreal x);
	void animFinished();

public:
	NodeTreeView(AIDebugger& debugger, AINodeStaticResolver& resolver, QWidget* parent = nullptr);
	virtual ~NodeTreeView();

	void wheelEvent(QWheelEvent * wheelEventPtr) override;

	void updateTreeWidget();
};

}
}
