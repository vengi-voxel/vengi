#pragma once

#include <QTableView>
#include <QSortFilterProxyModel>

#include "AIDebugger.h"
#include "StateTableModel.h"

namespace ai {
namespace debug {

/**
 * @brief Shows a key value pair of values for the selected entity
 */
class StateTable: public QTableView {
private:
	StateTableModel _model;
	QSortFilterProxyModel _proxyModel;
	AIDebugger& _debugger;
public:
	StateTable(AIDebugger& debugger);
	virtual ~StateTable();

	void updateStateTable();
};

}
}
