/**
 * @file
 */

#pragma once

#include <QTableView>
#include <QSortFilterProxyModel>
#include <QLineEdit>

#include "AIDebugger.h"
#include "EntityListModel.h"

namespace ai {
namespace debug {

/**
 * @brief Shows a list of all entities that are handled on the server we are connected to
 */
class EntityList: public QTableView {
Q_OBJECT
private:
	EntityListModel _model;
	QSortFilterProxyModel _proxyModel;
	AIDebugger& _debugger;
	QLineEdit* _entityFilter;
private slots:
	void selectEntity(const QModelIndex &current, const QModelIndex &previous);
public:
	EntityList(AIDebugger& debugger, QLineEdit *entityFilter);
	virtual ~EntityList();

	void updateEntityList();
};

}
}
