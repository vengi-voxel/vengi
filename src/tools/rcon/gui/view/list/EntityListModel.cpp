/**
 * @file
 */
#include "EntityListModel.h"
#include "AIDebugger.h"
#include <QColor>

namespace ai {
namespace debug {

EntityListModel::EntityListModel(AIDebugger& debugger, QTableView *tableView) :
		QAbstractTableModel(tableView), _debugger(debugger), _parent(tableView) {
}

EntityListModel::~EntityListModel() {
}

QModelIndex EntityListModel::characterIndex(CharacterId id) const {
	int row = 0;
	for (const AIStateWorld& state : _list) {
		if (state.getId() == id) {
			return createIndex(row, 0);
		}
		++row;
	}
	qDebug() << "Could not find entity " << id << " in the model";
	return QModelIndex();
}

void EntityListModel::update() {
	beginResetModel();
	_list = _debugger.getEntities().values();
	// TODO: sort list - not model
	endResetModel();
}

int EntityListModel::rowCount(const QModelIndex & /*parent*/) const {
	return _list.size();
}

int EntityListModel::columnCount(const QModelIndex & /*parent*/) const {
	return 1;
}

QVariant EntityListModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation != Qt::Horizontal) {
		return QVariant();
	}
	if (section != 0) {
		return QVariant();
	}

	if (role == Qt::DisplayRole) {
		return tr("Entities");
	}
	if (role == Qt::ToolTipRole) {
		return tr("The character id");
	}
	return QVariant();
}

QVariant EntityListModel::data(const QModelIndex &mdlIndex, int role) const {
	const AIStateWorld& state = _list.at(mdlIndex.row());
	if (role == Qt::DisplayRole) {
		if (mdlIndex.column() == 0) {
			const CharacterAttributes& attributes = state.getAttributes();
			auto name = attributes.find(attributes::NAME);
			if (name != attributes.end()) {
				return QString(name->second.c_str()).append(" (%1)").arg(state.getId());
			}
			return state.getId();
		}
	} else if (role == Qt::BackgroundRole) {
		if (_debugger.isSelected(state)) {
			return QColor(Qt::gray);
		}
	}
	return QVariant();
}

}
}
