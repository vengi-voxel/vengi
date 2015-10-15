#include "EntityList.h"
#include <QHeaderView>

namespace ai {
namespace debug {

EntityList::EntityList(AIDebugger& debugger, QLineEdit* entityFilter) :
		QTableView(), _model(debugger, this), _debugger(debugger), _entityFilter(entityFilter) {
	setFixedWidth(200);
	_proxyModel.setSourceModel(&_model);
	setModel(&_proxyModel);
	setAlternatingRowColors(true);
	setSortingEnabled(true);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
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
	_model.sort(0);
}

void EntityList::selectEntity(const QModelIndex &current, const QModelIndex &previous) {
	Q_UNUSED(previous);
	const AIStateWorld& state = _model.getEntities().at(_proxyModel.mapToSource(current).row());
	_debugger.select(state);
}

}
}
