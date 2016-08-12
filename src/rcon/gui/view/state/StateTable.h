/**
 * @file
 */
#pragma once

#include <QTableView>
#include <QSortFilterProxyModel>

#include "StateTableModel.h"

namespace ai {
namespace debug {

class AIDebugger;

/**
 * @brief Shows a key value pair of values for the selected entity
 */
class StateTable: public QTableView {
private:
	StateTableModel _model;
	QSortFilterProxyModel _proxyModel;
public:
	StateTable(AIDebugger& debugger);
	virtual ~StateTable();

	void updateStateTable();
};

}
}
