/**
 * @file
 */
#include "EntityList.h"
#include "AIDebugger.h"
#include <QHeaderView>

namespace ai {
namespace debug {

EntityList::EntityList(AIDebugger& debugger, QLineEdit* entityFilter) :
		_model(debugger, this), _debugger(debugger), _entityFilter(entityFilter) {
	setFixedWidth(200);
	_proxyModel.setSourceModel(&_model);
	setModel(&_proxyModel);
	setAlternatingRowColors(true);
	setSortingEnabled(false);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
	verticalHeader()->hide();
	horizontalHeader()->setStretchLastSection(true);

	connect(_entityFilter, SIGNAL(textChanged(QString)), &_proxyModel, SLOT(setFilterWildcard(QString)));
	connect(selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(selectEntity(QModelIndex,QModelIndex)));
}

EntityList::~EntityList() {
}

void EntityList::updateEntityList() {
	_model.update();
}

void EntityList::selectEntity(const QModelIndex &current, const QModelIndex &previous) {
	Q_UNUSED(previous);
	if (!current.isValid()) {
		return;
	}
	const QModelIndex index = _proxyModel.mapToSource(current);
	const AIStateWorld& worldState = _model.getEntities().at(index.row());
	_debugger.select(worldState);
}

}
}
