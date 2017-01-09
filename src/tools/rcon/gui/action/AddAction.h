/**
 * @file
 */
#include "Action.h"
#include "AddDialog.h"

namespace ai {
namespace debug {

class BehaviourTreeModelItem;

class AddAction: public Action {
Q_OBJECT
private:
	int _parentId;

private slots:
	void onTriggered() {
		AddDialog d;
		d.run();
		emit triggered(_parentId, d.getName(), d.getType(), d.getCondition());
	}
public:
	AddAction(int parentId, QObject* parentObj) :
			Action(tr("Add node"), parentObj), _parentId(parentId) {
		connect(this, SIGNAL(triggered()), this, SLOT(onTriggered()));
	}
signals:
	void triggered(int parentId, const QVariant& name, const QVariant& type, const QVariant& condition);
};

}
}
