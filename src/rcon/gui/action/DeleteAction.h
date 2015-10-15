#include <QAction>
#include <QModelIndex>
#include "AddDialog.h"

namespace ai {
namespace debug {

class BehaviourTreeModelItem;

class DeleteAction: public QAction {
Q_OBJECT
private:
	int _nodeId;
private slots:
	void onTriggered() {
		emit triggered(_nodeId);
	}
public:
	DeleteAction(int nodeId, QObject* parent) :
			QAction(parent), _nodeId(nodeId) {
		setText(tr("Delete node"));
		connect(this, SIGNAL(triggered()), this, SLOT(onTriggered()));
	}
signals:
	void triggered(int nodeId);
};

}
}
