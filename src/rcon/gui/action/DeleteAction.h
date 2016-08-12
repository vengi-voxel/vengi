/**
 * @file
 */
#include "Action.h"
#include "AddDialog.h"

namespace ai {
namespace debug {

class BehaviourTreeModelItem;

class DeleteAction: public Action {
Q_OBJECT
private:
	int _nodeId;
private slots:
	void onTriggered() {
		emit triggered(_nodeId);
	}
public:
	DeleteAction(int nodeId, QObject* parent) :
			Action(tr("Delete node"), parent), _nodeId(nodeId) {
		connect(this, SIGNAL(triggered()), this, SLOT(onTriggered()));
	}
signals:
	void triggered(int nodeId);
};

}
}
