#include "StateTable.h"
#include <QHeaderView>

namespace ai {
namespace debug {

StateTable::StateTable(AIDebugger& debugger) :
		QTableView(), _model(debugger) {
	_proxyModel.setSourceModel(&_model);
	setModel(&_proxyModel);
	setAlternatingRowColors(true);
	resizeColumnsToContents();
	setSortingEnabled(true);
	setSelectionMode(QAbstractItemView::NoSelection);
	verticalHeader()->hide();
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
	for (int c = 0; c < horizontalHeader()->count(); ++c) {
		horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
	}
}

StateTable::~StateTable() {
}

void StateTable::updateStateTable() {
	_model.update();
}

}
}
