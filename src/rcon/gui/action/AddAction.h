/**
 * @file
 */

#include <QAction>
#include <QModelIndex>
#include "AddDialog.h"

namespace ai {
namespace debug {

class BehaviourTreeModelItem;

class AddAction: public QAction {
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
	AddAction(int parentId, QObject* parent) :
			QAction(parent), _parentId(parentId) {
		setText(tr("Add node"));
		connect(this, SIGNAL(triggered()), this, SLOT(onTriggered()));
	}
signals:
	void triggered(int parentId, const QVariant& name, const QVariant& type, const QVariant& condition);
};

}
}
