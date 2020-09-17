/**
 * @file
 */
#include "AggroTable.h"
#include <QHeaderView>

namespace ai {
namespace debug {

AggroTable::AggroTable(AIDebugger& debugger) :
		_model(debugger, this), _debugger(debugger) {
	_proxyModel.setSourceModel(&_model);
	setModel(&_proxyModel);
	setAlternatingRowColors(true);
	resizeColumnsToContents();
	setSortingEnabled(false);
	setSelectionMode(QAbstractItemView::NoSelection);
	verticalHeader()->hide();
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
	for (int c = 0; c < horizontalHeader()->count(); ++c) {
		horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
	}
	connect(selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(selectEntity(QModelIndex,QModelIndex)));
}

AggroTable::~AggroTable() {
}

void AggroTable::updateAggroTable() {
	_model.update();
}

void AggroTable::selectEntity(const QModelIndex &current, const QModelIndex &previous) {
	Q_UNUSED(previous);
	if (!current.isValid()) {
		return;
	}
	const QModelIndex index = _proxyModel.mapToSource(current);
	const core::DynamicArray<AIStateAggroEntry>& aggro = _debugger.getAggro();
	const AIStateAggroEntry& aggroState = aggro[index.row()];
	_debugger.select(aggroState.id);
}

}
}
