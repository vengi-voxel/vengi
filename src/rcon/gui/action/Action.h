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
	Action(const QString& title, QObject* parent = nullptr) :
			QAction(title, parent) {
	}
	Action(QObject* parent = nullptr) :
			QAction(parent) {
	}

	// embedd stuff like e.g. a color dialog into a menu
	void setPopupDialog(QDialog* dialog) {
		QWidgetAction* action = new QWidgetAction(nullptr);
		action->setDefaultWidget(dialog);

		QMenu* menu = new QMenu();
		menu->addAction(action);
		connect(menu, SIGNAL(aboutToShow()), dialog, SLOT(show()));
		connect(dialog, SIGNAL(finished(int)), menu, SLOT(hide()));
		setMenu(menu);
	}
};

}
}
