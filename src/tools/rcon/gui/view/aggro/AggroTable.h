/**
 * @file
 */
#pragma once

#include <QTableView>
#include <QSortFilterProxyModel>

#include "AIDebugger.h"
#include "AggroTableModel.h"

namespace ai {
namespace debug {

/**
 * @brief Shows a key value pair of values for the selected entity
 */
class AggroTable: public QTableView {
Q_OBJECT
private:
	AggroTableModel _model;
	QSortFilterProxyModel _proxyModel;
	AIDebugger& _debugger;
private slots:
	void selectEntity(const QModelIndex &current, const QModelIndex &previous);
public:
	AggroTable(AIDebugger& debugger);
	virtual ~AggroTable();

	void updateAggroTable();
};

}
}
