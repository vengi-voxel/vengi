/**
 * @file
 */
#pragma once

#include <QAction>
#include <QDialog>
#include <QWidgetAction>
#include <QMenu>

namespace ai {
namespace debug {

class Action: public QAction {
Q_OBJECT
protected:
	Action(const QString& title, QObject* parentObj = nullptr) :
			QAction(title, parentObj) {
	}
	Action(QObject* parentObj = nullptr) :
			QAction(parentObj) {
	}

	// embedd stuff like e.g. a color dialog into a menu
	void setPopupDialog(QDialog* dialog) {
		QWidgetAction* action = new QWidgetAction(nullptr);
		action->setDefaultWidget(dialog);

		QMenu* actionMenu = new QMenu();
		actionMenu->addAction(action);
		connect(actionMenu, SIGNAL(aboutToShow()), dialog, SLOT(show()));
		connect(dialog, SIGNAL(finished(int)), actionMenu, SLOT(hide()));
		setMenu(actionMenu);
	}
};

}
}
