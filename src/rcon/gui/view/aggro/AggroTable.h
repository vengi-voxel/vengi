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
private:
	AggroTableModel _model;
	QSortFilterProxyModel _proxyModel;
public:
	AggroTable(AIDebugger& debugger);
	virtual ~AggroTable();

	void updateAggroTable();
};

}
}
